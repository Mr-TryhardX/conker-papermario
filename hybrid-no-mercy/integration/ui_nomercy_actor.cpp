#include "ui_nomercy_actor.h"
#include "nomercy_audio.h"
#include "recomp_ui.h"
#include "hh_support.h"
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/Geometry.h>
#include <json.hpp>
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>
namespace {
using Json=nlohmann::json;
Json poses;
bool loaded=false;
}
void recompui::register_nomercy_actor_images(){
    if(loaded)return;
    try {
        std::ifstream file(hybridheaven::get_asset_path("nomercy/actor/poses.json"));
        if(!file)return;file>>poses;
        const auto& frames=poses.at("poses");
        if(frames.empty()||frames.size()>120)throw std::runtime_error("Invalid pose count");
        for(const auto& part:frames[0].at("parts")){
            const auto name=part.at("texture").get<std::string>();
            if(name.find('/')!=std::string::npos||name.find("..")!=std::string::npos)throw std::runtime_error("Invalid texture name");
            const auto path=hybridheaven::get_asset_path(("nomercy/actor/"+name).c_str());
            std::ifstream image(path,std::ios::binary|std::ios::ate);
            const auto length=image.tellg();if(length<=0||length>4*1024*1024)throw std::runtime_error("Missing actor texture");
            image.seekg(0);std::vector<char> bytes(static_cast<size_t>(length));if(!image.read(bytes.data(),length))throw std::runtime_error("Short actor texture");
            recompui::queue_image_from_bytes_file("?/nomercy/actor/"+name,bytes);
        }
        loaded=true;fprintf(stderr,"[nomercy-actor] loaded %zu captured reference poses\n",frames.size());
    }catch(const std::exception& e){fprintf(stderr,"[nomercy-actor] %s\n",e.what());}
}
recompui::ElementNoMercyActor::ElementNoMercyActor(const Rml::String& tag):Rml::Element(tag){}
void recompui::ElementNoMercyActor::OnRender(){
    if(!loaded)return;nomercy_audio_heartbeat();
    const auto size=GetBox().GetSize(Rml::BoxArea::Content);if(size.x<10||size.y<10)return;
    const auto& frames=poses.at("poses");
    const auto& parts=frames[size_t(SDL_GetTicks64()/120)%frames.size()].at("parts");
    struct Part {size_t index;float depth;};std::vector<Part> order;
    // Fixed actor-space bounds avoid apparent changes in scale between frames.
    const float scale=std::min(size.x/120.0f,size.y/190.0f);
    for(size_t i=0;i<parts.size();++i){float depth=0;const auto& vs=parts[i].at("vertices");for(const auto& v:vs)depth+=v[2].get<float>();if(!vs.empty())order.push_back({i,-depth/vs.size()});}
    std::sort(order.begin(),order.end(),[](auto a,auto b){return a.depth<b.depth;});
    for(const auto& p:order){
        const auto& part=parts[p.index];const auto& vs=part.at("vertices");const auto& faces=part.at("faces");
        if(vs.size()>128||faces.size()>256)return;
        struct Tri {int a,b,c;float depth;};std::vector<Tri> sorted;
        for(const auto& face:faces){const int a=face[0],b=face[1],c=face[2];if(a<0||b<0||c<0||size_t(std::max({a,b,c}))>=vs.size())return;sorted.push_back({a,b,c,-(vs[a][2].get<float>()+vs[b][2].get<float>()+vs[c][2].get<float>())/3});}
        std::sort(sorted.begin(),sorted.end(),[](auto a,auto b){return a.depth<b.depth;});
        Rml::Mesh mesh;
        for(const auto& tri:sorted)for(int index:{tri.a,tri.b,tri.c}){
            const auto& v=vs[index];Rml::Vertex vertex;
            vertex.position={size.x*.5f-v[0].get<float>()*scale,size.y*.5f-(v[1].get<float>()-85)*scale};
            vertex.tex_coord={v[3].get<float>(),v[4].get<float>()};vertex.colour={255,255,255,255};
            mesh.indices.push_back(int(mesh.vertices.size()));mesh.vertices.push_back(vertex);
        }
        auto texture=GetRenderManager()->LoadTexture("?/nomercy/actor/"+part.at("texture").get<std::string>());
        auto geometry=GetRenderManager()->MakeGeometry(std::move(mesh));geometry.Render(GetAbsoluteOffset(Rml::BoxArea::Content),texture);
    }
}
