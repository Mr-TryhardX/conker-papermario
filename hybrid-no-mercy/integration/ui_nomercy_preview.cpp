#include "ui_nomercy_preview.h"
#include "ui_nomercy_actor.h"
#include "nomercy_audio.h"
#include "recomp_ui.h"
#include "hh_support.h"
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/Geometry.h>
#include <json.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>
#include <SDL.h>
namespace {
using Json=nlohmann::json;
int selected=0;
struct Assets {
    Json catalogue;
    bool loaded=false,failed=false;
    void load() {
        if(loaded||failed)return;
        try {
            std::ifstream file("assets/nomercy/catalogue.json");
            if(!file)throw std::runtime_error("No Mercy catalogue missing");
            file>>catalogue;
            if(catalogue.at("rom_sha1")!="91cee3d096f4a76644d8b35b9aead6448909abd1")throw std::runtime_error("Wrong asset source");
            if(catalogue.at("preview_meshes").size()!=20)throw std::runtime_error("Incomplete mesh import");
            loaded=true;
            fprintf(stderr,"[nomercy-import] loaded real AKI assets: %zu meshes, %zu animations, %zu costumes\n",catalogue.at("meshes").size(),catalogue.at("animations").size(),catalogue.at("costumes").size());
        }catch(const std::exception& e){failed=true;fprintf(stderr,"[nomercy-import] %s\n",e.what());}
    }
} assets;
}
void recompui::register_nomercy_images(){
    register_nomercy_actor_images();
    static bool tried=false;if(tried)return;tried=true;
    for(const char* id:{"45B6","2650","1893"}){
        const auto path=hybridheaven::get_asset_path((std::string("nomercy/")+id+".png").c_str());
        std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file){fprintf(stderr,"[nomercy-import] missing image %s\n",id);continue;}
        const auto size=file.tellg();if(size<=0||size>8*1024*1024)continue;
        file.seekg(0);std::vector<char> bytes(static_cast<size_t>(size));if(!file.read(bytes.data(),size))continue;
        recompui::queue_image_from_bytes_file(std::string("?/nomercy/")+id,bytes);
        fprintf(stderr,"[nomercy-import] registered original image %s (%zu bytes)\n",id,bytes.size());
    }
}
recompui::ElementNoMercyPreview::ElementNoMercyPreview(const Rml::String& tag):Rml::Element(tag){}
void recompui::nomercy_preview_select(int index){if(index>=0&&index<20)selected=index;}
void recompui::ElementNoMercyPreview::OnRender(){
    assets.load();if(!assets.loaded)return;
    nomercy_audio_heartbeat();
    const auto size=GetBox().GetSize(Rml::BoxArea::Content);
    if(size.x<10||size.y<10)return;
    char id[8];snprintf(id,sizeof(id),"%04X",0x110a+selected);
    const auto& mesh=assets.catalogue.at("preview_meshes").at(id);
    const auto& vertices=mesh.at("vertices");const auto& faces=mesh.at("faces");
    if(vertices.size()>127||faces.size()>127)return;
    std::vector<std::array<float,3>> v;
    std::array<float,3> low{1e9,1e9,1e9},high{-1e9,-1e9,-1e9};
    for(const auto& x:vertices){std::array<float,3>a{};for(int i=0;i<3;++i){a[i]=x.at(i).get<float>();low[i]=std::min(low[i],a[i]);high[i]=std::max(high[i],a[i]);}v.push_back(a);}
    const float extent=std::max({high[0]-low[0],high[1]-low[1],high[2]-low[2],1.0f});
    const float angle=float(SDL_GetTicks64()%20000)*6.2831853f/20000.0f;
    const float s=std::sin(angle),c=std::cos(angle),scale=std::min(size.x,size.y)*0.68f;
    for(auto& a:v){for(int i=0;i<3;++i)a[i]=(a[i]-(high[i]+low[i])*0.5f)/extent;const float x=a[0]*c+a[2]*s,z=-a[0]*s+a[2]*c;a[0]=x;a[2]=z;}
    struct Tri {int a,b,c;float z;};std::vector<Tri> triangles;
    for(const auto& f:faces){const int a=f[0],b=f[1],c=f[2];if(a<0||b<0||c<0||size_t(std::max({a,b,c}))>=v.size())return;triangles.push_back({a,b,c,(v[a][2]+v[b][2]+v[c][2])/3});}
    std::sort(triangles.begin(),triangles.end(),[](const auto&a,const auto&b){return a.z<b.z;});
    Rml::Mesh output;
    for(const auto& t:triangles){
        const auto a=v[t.a],b=v[t.b],cc=v[t.c];
        const std::array<float,3> u{b[0]-a[0],b[1]-a[1],b[2]-a[2]},w{cc[0]-a[0],cc[1]-a[1],cc[2]-a[2]};
        const float nx=u[1]*w[2]-u[2]*w[1],ny=u[2]*w[0]-u[0]*w[2],nz=u[0]*w[1]-u[1]*w[0];
        const float len=std::max(0.0001f,std::sqrt(nx*nx+ny*ny+nz*nz));
        const float light=0.35f+0.65f*std::abs((nx*0.2f+ny*0.5f+nz*0.84f)/len);
        const auto shade=uint8_t(std::clamp(light*255.0f,0.0f,255.0f));
        for(int index:{t.a,t.b,t.c}){
            const auto pos=v[index];const float perspective=2.8f/(2.8f-pos[2]);
            Rml::Vertex vertex;vertex.position={size.x*0.5f+pos[0]*scale*perspective,size.y*0.5f-pos[1]*scale*perspective};
            vertex.colour={shade,uint8_t(shade*0.35f),uint8_t(shade*0.3f),255};vertex.tex_coord={0,0};
            output.indices.push_back(int(output.vertices.size()));output.vertices.push_back(vertex);
        }
    }
    auto geometry=GetRenderManager()->MakeGeometry(std::move(output));geometry.Render(GetAbsoluteOffset(Rml::BoxArea::Content));
}
