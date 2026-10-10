#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "career.hpp"
#include "wave_music.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <functional>
#include <fstream>
#include <random>

namespace career {
bool replace_save_file(const std::filesystem::path& from,const std::filesystem::path& to) {
    return MoveFileExW(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
}
}
namespace {
constexpr float pi=3.14159265f;
struct V {float x,y,z;};
struct Area {float x0,x1,z0,z1;};
constexpr Area areas[]={{-6,6,-5,5},{-2,2,5,14},{-7,7,14,27},{0,4,27,35},{-8,8,35,51}};
constexpr V stations[]={{0,0,2},{-3,0,18},{3,0,24},{2,0,37},{-4,0,45},{3,0,48}};
career::Game game, pending;
std::filesystem::path folder,savefile;
std::string message="Walk toward the red marker. E starts the first encounter.",track;
bool writable=true,muted=false,keys[256]{},running=true;
HWND window{};HDC dc{};HGLRC context{};GLuint font{};GLUquadric* quad{};
int width=1280,height=800,menu=0,opponent=0,action=-1,preview_fighter=5;
float px=0,pz=-2,heading=0,camera=0,clocktime=0,walk=0,animation=0;
V stage{};float oldpx=0,oldpz=0;
V camera_eye{},camera_target{};
bool walking=false,impact=false,counter=false,has_result=false,practice=false;
std::string result;
std::mt19937 rng(std::random_device{}());
WaveMusic audio;
struct Button {int x,y,w,h,id;};std::vector<Button> buttons;

float rad(float d){return d*pi/180.f;}
float clamp(float v,float a,float b){return std::max(a,std::min(v,b));}
float distance(float x,float z,V v){return std::hypot(x-v.x,z-v.z);}
const char* finisher_name(int who){static const char* names[]={"Spinning Kick","Stunner","Rock Bottom","Angle Slam","Tombstone","Chokeslam"};return names[who];}
int visual_fighter(){return practice?preview_fighter:game.state.selected;}
void color(float r,float g,float b){glColor3f(r,g,b);}
void music(bool battle){
    auto path=folder/L"assets/audio/no-mercy-shop.wav";track="No Mercy shop";
    if(battle){std::vector<std::filesystem::path> list;auto dir=folder/L"assets/audio/entrances";
        if(std::filesystem::exists(dir))for(const auto& e:std::filesystem::directory_iterator(dir))
            if(e.is_regular_file()&&e.path().extension()==L".wav")list.push_back(e.path());
        if(!list.empty()){path=list[std::uniform_int_distribution<size_t>(0,list.size()-1)(rng)];track=path.stem().string();}
        else {path.clear();track="No entrance clips found";}
    }
    bool exists=!path.empty()&&std::filesystem::exists(path);
    bool played=false;std::string failure;
    if(!muted&&exists)played=audio.play(path,failure);
    else audio.stop();
    std::ofstream log(folder/L"audio-status.txt",std::ios::app);auto utf8_path=path.u8string();
    log<<"track="<<track<<" file="<<std::string(utf8_path.begin(),utf8_path.end())<<" exists="<<exists<<" muted="<<muted<<" playback="<<played<<" error="<<failure<<"\n";
    if(!muted&&!exists){track="Music files missing";message="Music missing: extract the whole ZIP and keep assets beside AnimatedDemo.exe.";}
    else if(!muted&&!played){track="Audio playback failed";message="Audio failed: "+failure;}
}
bool persist(const career::Game& before){std::string error;if(game.save(savefile,error))return true;game=before;message="Could not save; action rolled back: "+error;return false;}
bool change(const std::function<bool(std::string&)>& fn){if(!writable){message="Save is unreadable. Back it up before resetting it.";return false;}auto before=game;if(fn(message))return persist(before);return false;}
void cube(float x,float y,float z,float sx,float sy,float sz){
    glPushMatrix();glTranslatef(x,y,z);glScalef(sx,sy,sz);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);glVertex3f(-.5,.5,-.5);glVertex3f(-.5,.5,.5);glVertex3f(.5,.5,.5);glVertex3f(.5,.5,-.5);
    glNormal3f(0,-1,0);glVertex3f(-.5,-.5,-.5);glVertex3f(.5,-.5,-.5);glVertex3f(.5,-.5,.5);glVertex3f(-.5,-.5,.5);
    glNormal3f(0,0,1);glVertex3f(-.5,-.5,.5);glVertex3f(.5,-.5,.5);glVertex3f(.5,.5,.5);glVertex3f(-.5,.5,.5);
    glNormal3f(0,0,-1);glVertex3f(-.5,-.5,-.5);glVertex3f(-.5,.5,-.5);glVertex3f(.5,.5,-.5);glVertex3f(.5,-.5,-.5);
    glNormal3f(1,0,0);glVertex3f(.5,-.5,-.5);glVertex3f(.5,.5,-.5);glVertex3f(.5,.5,.5);glVertex3f(.5,-.5,.5);
    glNormal3f(-1,0,0);glVertex3f(-.5,-.5,-.5);glVertex3f(-.5,-.5,.5);glVertex3f(-.5,.5,.5);glVertex3f(-.5,.5,-.5);
    glEnd();glPopMatrix();
}
void ellipsoid(float x,float y,float z,float a,float b,float c){glPushMatrix();glTranslatef(x,y,z);glScalef(a,b,c);gluSphere(quad,1,12,8);glPopMatrix();}
void limb(float length,float radius){glPushMatrix();glRotatef(90,1,0,0);gluCylinder(quad,radius,radius*.8f,length,10,1);glPopMatrix();ellipsoid(0,-length,0,radius,radius,radius);}
struct Pose {float armL=0,armR=0,legL=0,legR=0,elbowL=-15,elbowR=-15,kneeL=0,kneeR=0,lean=0,rootY=0,rootZ=0,roll=0;};
void wrestler(int who,int costume,float x,float z,float yaw,const Pose& pose,bool enemy=false){
    float skin[3]={.71f,.47f,.34f};if(who==2){skin[0]=.52f;skin[1]=.32f;skin[2]=.20f;}
    float suit[4][3]={{.70f,.025f,.04f},{.08f,.16f,.70f},{.80f,.79f,.68f},{.17f,.025f,.03f}};
    float dark_suit[4][3]={{.025f,.028f,.034f},{.29f,.07f,.42f},{.55f,.57f,.59f},{.07f,.12f,.24f}};
    float* s=who==4?dark_suit[costume%4]:suit[costume%4];if(enemy){s=suit[1];skin[0]=.48f;skin[1]=.54f;skin[2]=.50f;}
    glPushMatrix();glTranslatef(x,pose.rootY,z);glRotatef(yaw,0,1,0);glTranslatef(0,0,pose.rootZ);glRotatef(pose.roll,0,0,1);
    float bulk=(who==5||who==4)?1.1f:1;glScalef(bulk,bulk,bulk);
    glTranslatef(0,1.15f,0);glRotatef(pose.lean,1,0,0);
    color(who==5||enemy?s[0]:skin[0],who==5||enemy?s[1]:skin[1],who==5||enemy?s[2]:skin[2]);
    ellipsoid(0,.30f,0,.43f,.53f,.25f);color(.035,.04,.045);cube(0,.0f,0,.69f,.20f,.44f);
    if(who!=5&&who!=4&&!enemy){color(s[0],s[1],s[2]);cube(0,-.035f,0,.73f,.28f,.48f);}
    if(who==5){color(.025,.025,.03);cube(-.13f,.36f,.245f,.17f,.66f,.025f);cube(.14f,.23f,.25f,.12f,.48f,.025f);}
    if(who==0||who==3){color(s[0],s[1],s[2]);cube(0,.32f,.22f,.52f,.58f,.06f);}
    color(skin[0],skin[1],skin[2]);ellipsoid(0,.95f,0,.23f,.29f,.22f);
    if(who==5){color(s[0],s[1],s[2]);ellipsoid(0,.96f,.14f,.231f,.25f,.095f);color(.025,.025,.025);cube(0,.98f,.232f,.35f,.06f,.03f);cube(0,.83f,.23f,.12f,.07f,.03f);}
    else {color(.015,.02,.025);cube(-.09f,1,.218f,.07f,.028f,.02f);cube(.09f,1,.218f,.07f,.028f,.02f);}
    if(who==5||who==4){color(.035,.023,.018);ellipsoid(0,1.13f,-.055f,.25f,.15f,.22f);cube(0,.83f,-.19f,.46f,.54f,.11f);}
    for(int side:{-1,1}){
        glPushMatrix();glTranslatef(side*.45f,.59f,0);glRotatef(side*-8.f,0,0,1);glRotatef(side<0?pose.armL:pose.armR,1,0,0);
        color(skin[0],skin[1],skin[2]);ellipsoid(0,0,0,.18f,.19f,.18f);limb(.43f,.14f);
        glTranslatef(0,-.43f,0);glRotatef(side<0?pose.elbowL:pose.elbowR,1,0,0);limb(.38f,.115f);
        color(.03,.035,.04);ellipsoid(0,-.40f,0,.13f,.15f,.13f);glPopMatrix();
        glPushMatrix();glTranslatef(side*.22f,-.11f,0);glRotatef(side<0?pose.legL:pose.legR,1,0,0);
        color((who==5||who==4||enemy)?s[0]:skin[0],(who==5||who==4||enemy)?s[1]:skin[1],(who==5||who==4||enemy)?s[2]:skin[2]);limb(.52f,.185f);
        glTranslatef(0,-.52f,0);glRotatef(side<0?pose.kneeL:pose.kneeR,1,0,0);color(.025,.03,.035);limb(.44f,.155f);cube(0,-.40f,.10f,.31f,.22f,.45f);glPopMatrix();
    }
    glPopMatrix();
}
bool walkable(float x,float z){for(const auto& a:areas)if(x>=a.x0+.30f&&x<=a.x1-.30f&&z>=a.z0-.05f&&z<=a.z1+.05f)return true;return false;}
void wall(float x,float z,float sx,float sz){
    // Cut away walls between the camera and the fighter instead of zooming into them.
    float lo=0,hi=1;float origin[]={camera_eye.x,camera_eye.z},direction[]={camera_target.x-camera_eye.x,camera_target.z-camera_eye.z},low[]={x-sx*.5f,z-sz*.5f},high[]={x+sx*.5f,z+sz*.5f};
    bool blocked=true;
    for(int i=0;i<2;++i){if(std::abs(direction[i])<.0001f){if(origin[i]<low[i]||origin[i]>high[i])blocked=false;}
        else {float a=(low[i]-origin[i])/direction[i],b=(high[i]-origin[i])/direction[i];if(a>b)std::swap(a,b);lo=std::max(lo,a);hi=std::min(hi,b);if(lo>hi)blocked=false;}}
    if(blocked&&hi>0&&lo<1)return;
    color(.10,.15,.18);cube(x,1.1f,z,sx,2.2f,sz);color(.04,.8,.64);cube(x,1.75f,z,sx+.02f,.055f,sz+.02f);
}
void facility(){
    for(const auto& a:areas){color(.13,.17,.19);cube((a.x0+a.x1)/2,-.15f,(a.z0+a.z1)/2,a.x1-a.x0,.3f,a.z1-a.z0);
        for(float z=a.z0+.5f;z<a.z1;z+=1){color(.19,.24,.25);cube((a.x0+a.x1)/2,.008f,z,a.x1-a.x0-.1f,.015f,.035f);}
        wall(a.x0,(a.z0+a.z1)/2,.2f,a.z1-a.z0);wall(a.x1,(a.z0+a.z1)/2,.2f,a.z1-a.z0);
    }
    wall(0,-5,12,.2);wall(-4,5,4,.2);wall(4,5,4,.2);wall(-4.5,14,5,.2);wall(4.5,14,5,.2);
    wall(-3.5,27,7,.2);wall(5.5,27,3,.2);wall(-4,35,8,.2);wall(6,35,4,.2);wall(0,51,16,.2);
    // Lab consoles, equipment and animated indicator lights.
    for(int i=0;i<3;++i){float x=-4.f+i*4; color(.06,.08,.10);cube(x,.65f,-3.7f,1.35f,1.3f,.7f);
        color(.08,.55f+.20f*std::sin(clocktime*2+i),.40);cube(x,1.22f,-3.32f,1.10f,.43f,.025f);}
    for(int i=0;i<8;++i){color(.16,.21,.24);cube(-6,.55f,15.5f+i*1.4f,1,.95f,.7f);color(.25,.33,.34);cube(6,.35f,16+i*1.1f,.8f,.7f,.6f);}
    for(int i=0;i<6;++i){auto v=stations[i];bool unlocked=i==0||game.state.cleared[i-1];bool cleared=game.state.cleared[i];
        glPushMatrix();glTranslatef(v.x,.035f,v.z);glRotatef(-90,1,0,0);color(cleared?.1f:unlocked?.85f:.2f,cleared?.8f:.16f,.15);gluDisk(quad,1.3,1.38,32,1);glPopMatrix();
        if(!game.battle.active&&!animation){Pose p;p.armL=-20;p.armR=-20;p.elbowL=-70;p.elbowR=-70;p.rootY=.025f*std::sin(clocktime*2+i);wrestler(0,1,v.x,v.z,180,p,true);}
    }
    color(.20,.34,.35);cube(-5,1,40,1,2,1);cube(6,1.25f,46,1.4f,2.5f,1.4f);
}
void rect(float x,float y,float w,float h,float r,float g,float b){color(r,g,b);glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();}
void text(float x,float y,const std::string& s,float r=.90,float g=.94,float b=.88){color(r,g,b);glRasterPos2f(x,y+16);glListBase(font);glCallLists(static_cast<GLsizei>(s.size()),GL_UNSIGNED_BYTE,s.c_str());}
void button(int id,int x,int y,int w,const std::string& s){buttons.push_back({x,y,w,34,id});rect(x,y,w,34,.16,.23,.17);text(x+8,y+6,s);}
void bar(int x,int y,int w,int value,int max,float r,float g,float b){rect(x,y,w,10,.14,.18,.17);rect(x,y,w*clamp(float(value)/std::max(1,max),0,1),10,r,g,b);}
void overlay(){
    glDisable(GL_LIGHTING);glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();buttons.clear();
    rect(0,0,width,91,.035,.055,.055);text(20,8,"FACILITY BREACH / ANIMATED WORLD PROTOTYPE",.5,.9,.35);
    const auto& f=game.state.roster[game.state.selected];
    text(20,32,std::string(practice?"Preview: ":"")+career::fighters[visual_fighter()].name+" | Lv "+std::to_string(f.level)+" | XP "+std::to_string(f.xp)+"/"+std::to_string(game.next_xp())+" | Credits "+std::to_string(game.state.coins)+" | Points "+std::to_string(f.points)+" | Attire "+std::to_string(f.costume+1));
    text(20,57,"Original rough models/animations. Imported No Mercy assets and Hybrid Heaven levels are pending.",.70,.76,.75);
    rect(0,height-87,width,87,.035,.055,.055);text(20,height-77,message.substr(0,125),.70,.92,.50);
    text(20,height-51,"WASD move | Arrows turn camera | E interact | F1 help | F2 roster | F3 train | F4 shop | M music");
    text(20,height-27,"Music: "+(muted?std::string("OFF"):track)+" | Progress saves automatically. Esc closes menus or retreats.",.66,.73,.73);
    if(game.battle.active||animation){rect(15,103,340,147,.055,.085,.075);text(28,111,career::encounters[opponent].name);
        text(28,137,"HP "+std::to_string(game.battle.hp)+" / "+std::to_string(game.max_hp()));bar(28,160,310,game.battle.hp,game.max_hp(),.45,.8,.2);
        text(28,176,"Stamina "+std::to_string(game.battle.stamina));bar(28,198,310,game.battle.stamina,game.max_stamina(),.15,.6,.85);
        text(28,214,"Opponent HP "+std::to_string(game.battle.enemy_hp));
        rect(width-330,103,315,258,.055,.085,.075);text(width-315,111,animation?"ANIMATING TURN...":practice?"PRACTICE / NO REWARDS":"CHOOSE YOUR MOVE");
        for(int i=0;i<8;++i)text(width-315,139+i*21,std::to_string(i+1)+"  "+(i==7?finisher_name(visual_fighter()):career::moves[i].name)+" / "+std::to_string(career::moves[i].stamina)+" ST",(practice||f.learned[i])?.85f:.38f,(practice||f.learned[i])?.90f:.42f,.65);
        text(width-315,316,"G guard | R recover | H medkit");text(width-315,337,practice?"Tab preview fighter | Esc exit":"Esc retreat");
    }else if(!menu){
        for(int i=1;i<=5;++i)button(i,20+(i-1)*125,101,115,i==1?"Help":i==2?"Roster":i==3?"Training":i==4?"Shop":"Records");
        button(6,645,101,175,"Practice moves");
        int nearby=-1;for(int i=0;i<6;++i)if(distance(px,pz,stations[i])<2.7f)nearby=i;
        if(nearby>=0){rect(width/2-230,height-145,460,40,.06,.10,.08);text(width/2-212,height-135,"E: "+std::string(career::encounters[nearby].name)+(nearby>0&&!game.state.cleared[nearby-1]?" [locked]":""));}
        if(pz<-2.5f){rect(width/2-260,height-145,520,40,.06,.10,.08);text(width/2-245,height-135,"Lab consoles: F2 roster / F3 training / F4 shop");}
        int done=static_cast<int>(std::count(game.state.cleared.begin(),game.state.cleared.end(),true));text(width-285,104,"Test chapter: "+std::to_string(done)+" / 6 cleared",.6,.9,.4);
        if(done==6){rect(width/2-260,150,520,43,.07,.17,.10);text(width/2-245,162,"TEST CHAPTER COMPLETE - replay or try another fighter",.65,.95,.40);}
    }
    if(menu){int left=std::max(10,(width-920)/2),top=102;rect(left,top,920,height-202,.06,.09,.08);
        for(int i=1;i<=5;++i)button(i,left+15+(i-1)*175,top+12,163,i==1?"Help":i==2?"Roster":i==3?"Training":i==4?"Shop":"Records");
        int x=left+25,y=top+65;
        if(menu==1){text(x,y,"Explore three rooms and the connecting corridors. Red circles mark encounter stations.");
            text(x,y+30,"Start close to a marker with E. Battles use the RPG rules, with animated actions.");
            text(x,y+60,"1-8 attacks, G guard, R recover, H medkit. Let each turn animation finish.");
            text(x,y+90,"Win to earn XP, credits, and training points. Buy moves and fighters between battles.");
            text(x,y+120,"Attire choices visibly change these original procedural models. Original costumes are pending.");
            text(x,y+150,"P: practice all moves now (7 powerbomb). F5 records. F10 entrance. F12 screenshot.");
            text(x,y+180,"No controller support yet. Mouse selects menus. Esc returns to exploration.");
        }else if(menu==2){text(x,y,"Choose a profile; paid unlocks use the shared wallet.");
            for(int i=0;i<6;++i){const auto& def=career::fighters[i];auto& c=game.state.roster[i];button(100+i,x,y+35+i*40,860,std::string(def.name)+" / "+def.style+" / "+(c.unlocked?"Lv "+std::to_string(c.level):std::to_string(def.price)+" credits"));}
            for(int i=0;i<4;++i)button(160+i,x+i*217,y+290,205,"Attire "+std::to_string(i+1)+(f.costume==i?" [active]":""));
            text(x,y+340,std::string("Perk: ")+career::fighters[game.state.selected].perk_detail);
        }else if(menu==3){text(x,y,"Spend points earned when this fighter levels up.");
            for(int i=0;i<6;++i)button(200+i,x+(i%2)*440,y+40+(i/2)*52,420,std::string(career::stat_names[i])+" "+std::to_string(game.stat(i))+" [+1]");
            text(x,y+230,"Training points: "+std::to_string(f.points));
        }else if(menu==4){text(x,y,"Moves belong to the selected fighter; medkits and credits are shared.");
            for(int i=3;i<8;++i)button(300+i,x,y+40+(i-3)*43,860,std::string(i==7?finisher_name(game.state.selected):career::moves[i].name)+" / Lv "+std::to_string(career::moves[i].required_level)+" / "+(f.learned[i]?"owned":std::to_string(career::moves[i].price)+" credits"));
            button(350,x,y+275,860,"Medkit / 25 credits / Inventory "+std::to_string(game.state.medkits));
        }else {text(x,y,"Wins "+std::to_string(game.state.wins)+" / Defeats "+std::to_string(game.state.losses));
            for(int i=0;i<6;++i)text(x,y+40+i*35,std::string(game.state.cleared[i]?"[CLEARED] ":"[PENDING] ")+career::encounters[i].name);
            text(x,y+290,"Save: LocalAppData / HybridNoMercyAnimatedDemo / career.json");}
    }
    if(has_result&&!animation){rect(width/2-270,115,540,67,.08,.15,.10);text(width/2-250,129,result.substr(0,65),.7,1,.5);text(width/2-250,155,"Continue exploring. F3 upgrades; F4 shop.");}
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);
}
void snapshot(){std::vector<unsigned char> pixels(static_cast<size_t>(width)*height*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(GL_BACK);glReadPixels(0,0,width,height,GL_BGR_EXT,GL_UNSIGNED_BYTE,pixels.data());
    std::vector<unsigned char> padded(static_cast<size_t>((width*3+3)&~3)*height);int stride=(width*3+3)&~3;for(int y=0;y<height;++y)std::copy_n(pixels.data()+static_cast<size_t>(y)*width*3,width*3,padded.data()+static_cast<size_t>(y)*stride);
    BITMAPFILEHEADER file{};BITMAPINFOHEADER info{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(info);file.bfSize=file.bfOffBits+static_cast<DWORD>(padded.size());info.biSize=sizeof(info);info.biWidth=width;info.biHeight=height;info.biPlanes=1;info.biBitCount=24;
    auto p=folder/L"AnimatedDemo-screenshot.bmp";HANDLE h=CreateFileW(p.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(h!=INVALID_HANDLE_VALUE){DWORD n;WriteFile(h,&file,sizeof(file),&n,nullptr);WriteFile(h,&info,sizeof(info),&n,nullptr);WriteFile(h,padded.data(),static_cast<DWORD>(padded.size()),&n,nullptr);CloseHandle(h);message="Screenshot saved beside the executable.";}
    nlohmann::json report={{"career",game.serialize()},{"x",px},{"z",pz},{"camera",camera},{"animation",animation},{"action",action},{"practice",practice},{"visual_fighter",visual_fighter()},{"battle",game.battle.active},{"enemy_hp",game.battle.enemy_hp},{"hp",game.battle.hp},{"stamina",game.battle.stamina},{"menu",menu},{"writable",writable},{"track",track},{"audio_queued",audio.playing()}};
    std::ofstream(folder/L"AnimatedDemo-diagnostics.json")<<report.dump(2);
}
void render(){
    glViewport(0,0,width,height);glClearColor(.025,.04,.055,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(58,double(width)/height,.15,110);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    float targetx=px,targetz=pz,dx=-std::sin(rad(camera)),dz=-std::cos(rad(camera)),desired=6.8f;
    bool fight=game.battle.active||animation;
    if(fight){targetx=stage.x;targetz=stage.z;desired=8.5f;dx=5.8f/8.5f;dz=-6.2f/8.5f;}
    float camera_distance=desired;
    float cx=targetx+dx*camera_distance,cz=targetz+dz*camera_distance;
    camera_eye={cx,2.3f+camera_distance*.23f,cz};camera_target={targetx,fight?1.f:1.15f,targetz};
    gluLookAt(cx,camera_eye.y,cz,targetx,camera_target.y,targetz,0,1,0);
    GLfloat light[]={-8,14,-2,1};glLightfv(GL_LIGHT0,GL_POSITION,light);facility();
    Pose p;float swing=walking?std::sin(walk)*32.f:std::sin(clocktime*1.8f)*2;
    p.armL=swing;p.armR=-swing;p.legL=-swing;p.legR=swing;p.rootY=walking?std::abs(std::sin(walk))*.06f:std::sin(clocktime*2)*.015f;
    float x=px,z=pz,yaw=heading;
    if(game.battle.active||animation){x=stage.x-1.1f;z=stage.z;yaw=90;p.armL=p.armR=-25;p.elbowL=p.elbowR=-65;Pose e=p;float ex=stage.x+1.1f,ez=stage.z;
        if(animation){float t=2.5f-animation;float attack=std::sin(clamp((t-.12f)/.85f,0,1)*pi),retaliate=std::sin(clamp((t-1.25f)/.7f,0,1)*pi);
            if(action>=0&&action<8){x+=attack*.48f;
                if(action==0){p.armR=-25-attack*75;p.elbowR=-65+attack*60;e.lean=-attack*24;}
                else if(action==1){p.legR=-attack*90;p.kneeR=attack*20;e.lean=-attack*35;}
                else if(action==6){
                    float lift=clamp((t-.15f)/.55f,0,1),slam=clamp((t-.85f)/.38f,0,1);
                    lift=lift*lift*(3-2*lift);slam=slam*slam*(3-2*slam);
                    p.armL=p.armR=-25-lift*115+slam*85;p.elbowL=p.elbowR=-25;p.lean=slam*38;p.legL=p.legR=-slam*18;
                    ex=stage.x+1.1f-lift*1.35f+slam*.75f;e.rootY=lift*1.30f-slam*1.02f;e.roll=slam*90;e.legL=e.legR=-lift*75*(1-slam);e.kneeL=e.kneeR=lift*70*(1-slam);e.armL=e.armR=-lift*65;
                }
                else if(action==7){
                    float lift=clamp((t-.12f)/.55f,0,1),drop=clamp((t-.90f)/.40f,0,1);
                    lift=lift*lift*(3-2*lift);drop=drop*drop*(3-2*drop);int who=visual_fighter();
                    if(who==5){p.armR=-25-lift*120+drop*90;p.elbowR=-10;ex-=lift*.80f;e.rootY=lift*1.55f-drop*1.27f;e.roll=drop*90;e.armL=e.armR=-lift*80;p.lean=drop*30;}
                    else if(who==4){p.armL=p.armR=-25-lift*85;p.elbowL=p.elbowR=-70;p.rootY=-drop*.35f;p.legL=p.legR=-drop*55;p.kneeL=p.kneeR=drop*80;ex-=lift*1.30f;e.roll=lift*180;e.rootY=lift*3.5f-drop*.95f;e.armL=e.armR=-lift*125;}
                    else if(who==1){p.armR=-25-lift*100;p.elbowR=-70;p.rootY=-drop*.6f;p.legL=p.legR=-drop*70;p.kneeL=p.kneeR=drop*70;p.lean=-drop*25;ex-=lift*1.45f;e.lean=lift*60;e.rootY=-drop*.30f;e.armL=e.armR=-lift*60;}
                    else if(who==2){p.armR=-25-lift*100;p.lean=drop*70;p.rootY=-drop*.35f;ex-=lift*.8f;e.roll=drop*90;e.rootY=lift*.25f+drop*.05f;e.armR=-lift*100;}
                    else if(who==3){p.armL=p.armR=-25-lift*100;p.lean=lift*20-drop*45;ex-=lift*1.6f;e.rootY=lift*1.35f-drop*1.07f;e.roll=lift*100+drop*170;e.legL=e.legR=-lift*35;}
                    else {yaw+=attack*360;p.legR=-attack*100;p.kneeR=attack*20;e.lean=-attack*40;}
                }
                else if(action==2||action==3){p.armL=p.armR=-25-attack*100;p.elbowL=p.elbowR=-30;p.lean=attack*25;ex-=attack*1.05f;e.rootY=attack*1.25f;e.roll=attack*105;e.rootZ=-attack*.40f;}
                else if(action==4){p.lean=attack*45;p.armR=-25-attack*80;e.roll=-attack*80;e.rootY=attack*.35f;}
                else {p.lean=attack*35;p.armL=p.armR=-25-attack*80;e.lean=attack*50;e.legL=e.legR=-attack*40;}
            }else if(action==8){p.armL=p.armR=-65;p.elbowL=p.elbowR=-100;}
            else if(action==10){p.armR=-65;p.elbowR=-115;}
            if(pending.battle.enemy_hp>0&&!practice){e.armR=-25-retaliate*80;e.elbowR=-65+retaliate*65;ex-=retaliate*.45f;p.lean=-retaliate*(action==8?7:23);}
            if(pending.battle.enemy_hp==0&&t>1){e.roll=90*clamp((t-1)/.5f,0,1);e.rootY=.28f*clamp((t-1)/.5f,0,1);}
        }
        wrestler(0,1,ex,ez,-90,e,true);
        if(impact){glDisable(GL_LIGHTING);color(1,.65,.2);glPushMatrix();glTranslatef(stage.x,1.3f,stage.z);glRotatef(clocktime*240,0,1,0);gluSphere(quad,.16,8,5);glPopMatrix();glEnable(GL_LIGHTING);}
    }
    wrestler(visual_fighter(),game.state.roster[game.state.selected].costume,x,z,yaw,p);overlay();SwapBuffers(dc);
}
void begin(){if(!writable){message="Save unreadable; back it up before resetting.";return;}
    int found=-1;float best=2.7f;for(int i=0;i<6;++i){float d=distance(px,pz,stations[i]);if(d<best){best=d;found=i;}}
    if(found<0){message="Approach an encounter marker, or use F2/F3/F4 at the lab consoles.";return;}
    auto before=game;if(game.begin(found,message)&&persist(before)){practice=false;opponent=found;stage=stations[found];oldpx=px;oldpz=pz;px=stage.x-1.1f;pz=stage.z;menu=0;has_result=false;music(true);}
}
void turn(int selected){if(!game.battle.active||animation)return;pending=game;bool ok=false;
    if(practice)ok=true;else if(selected<8)ok=pending.attack(selected,message);else if(selected==8||selected==9)ok=pending.recover(selected==8,message);else ok=pending.heal(message);
    if(ok){action=selected;animation=2.5f;impact=false;counter=false;message=selected<8?std::string("Animating ")+(selected==7?finisher_name(visual_fighter()):career::moves[selected].name):selected==8?"Guarding...":selected==9?"Recovering...":"Using medkit...";}
}
void tick(float dt){clocktime+=dt;if(animation){float prior=animation;animation=std::max(0.f,animation-dt);impact=animation<1.95f&&animation>1.55f;counter=animation<.9f&&animation>.5f;
        if(prior>0&&animation==0){if(practice){message="Practice animation complete. No items consumed or rewards earned. Try another move.";return;}
            auto before=game;game=pending;if(!persist(before)){has_result=false;return;}message=game.battle.log.empty()?"Turn complete.":game.battle.log.back();
            if(!game.battle.active){has_result=true;result=game.battle.hp>0?"Victory! Career progress saved.":"Defeat. Upgrades retained. Try again.";px=oldpx;pz=oldpz;music(false);}}
        return;}
    if(menu||game.battle.active){walking=false;return;}
    if(keys[VK_LEFT])camera-=90*dt;
    if(keys[VK_RIGHT])camera+=90*dt;
    float forward=(keys['W']?1.f:0)-(keys['S']?1.f:0),side=(keys['D']?1.f:0)-(keys['A']?1.f:0),length=std::hypot(forward,side);walking=length>0;
    if(walking){forward/=length;side/=length;float dx=std::sin(rad(camera))*forward+std::cos(rad(camera))*side,dz=std::cos(rad(camera))*forward-std::sin(rad(camera))*side;
        float speed=keys[VK_SHIFT]?4.8f:3.0f;float nx=px+dx*speed*dt,nz=pz+dz*speed*dt;if(walkable(nx,pz))px=nx;if(walkable(px,nz))pz=nz;heading=std::atan2(dx,dz)*180/pi;walk+=dt*speed*2.8f;has_result=false;
    }
}
void command(int id){if(id==VK_F12){snapshot();return;}if(animation)return;if(game.battle.active){if(id==VK_ESCAPE){auto before=game;game.retreat(message);if(practice||persist(before)){practice=false;px=oldpx;pz=oldpz;music(false);}return;}
        if(practice&&id==VK_TAB){preview_fighter=(preview_fighter+1)%career::fighter_count;message=std::string("Preview: ")+career::fighters[preview_fighter].name+". Press 8 for "+finisher_name(preview_fighter);return;}
        if(id>='1'&&id<='8')turn(id-'1');
        if(id=='G')turn(8);
        if(id=='R')turn(9);
        if(id=='H')turn(10);
        if(id=='M'){muted=!muted;music(true);}
        if(id==VK_F12)snapshot();
        return;}
    if(id==VK_ESCAPE){menu=0;return;}if(id>=VK_F1&&id<=VK_F5){menu=menu==id-VK_F1+1?0:id-VK_F1+1;has_result=false;return;}
    if(id=='E'&&!menu)begin();
    if(id=='P'&&!menu){practice=true;preview_fighter=game.state.selected;opponent=0;stage=stations[0];oldpx=px;oldpz=pz;px=stage.x-1.1f;pz=stage.z;game.battle=career::Battle{};game.battle.active=true;game.battle.hp=game.max_hp();game.battle.stamina=game.max_stamina();game.battle.enemy_hp=999;has_result=false;music(true);message="Practice: 7 powerbomb, 8 finisher, Tab preview fighter. No rewards or item use.";}
    if(id=='M'){muted=!muted;music(game.battle.active);}
    if(id==VK_F10){px=0;pz=-2;camera=0;message="Returned to the lab entrance.";}
    if(id==VK_F12)snapshot();
}
void click(int x,int y){if(game.battle.active||animation)return;for(const auto& b:buttons)if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h){
    if(b.id<=5){menu=b.id;return;}if(b.id==6){command('P');return;}if(b.id>=100&&b.id<106){if(change([&](std::string& s){return game.select(b.id-100,s);}))message=std::string(career::fighters[game.state.selected].name)+" selected. Original prototype model.";}
    else if(b.id>=160&&b.id<164){if(change([&](std::string& s){return game.select_costume(b.id-160,s);}))message="Prototype attire variant selected. Original No Mercy costumes are pending.";}
    else if(b.id>=200&&b.id<206)change([&](std::string& s){return game.upgrade(b.id-200,s);});
    else if(b.id>=303&&b.id<308)change([&](std::string& s){return game.buy_move(b.id-300,s);});else if(b.id==350)change([&](std::string& s){return game.buy_medkit(s);});
    return;}
}
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_SIZE:width=std::max(1,int(LOWORD(l)));height=std::max(1,int(HIWORD(l)));return 0;
    case WM_KEYDOWN:if(w<256){keys[w]=true;if(!(l&(1ll<<30)))command(static_cast<int>(w));}return 0;
    case WM_KEYUP:if(w<256)keys[w]=false;return 0;
    case WM_KILLFOCUS:std::fill(std::begin(keys),std::end(keys),false);return 0;
    case WM_LBUTTONDOWN:click(static_cast<short>(LOWORD(l)),static_cast<short>(HIWORD(l)));return 0;
    case WM_CLOSE:DestroyWindow(h);return 0;case WM_DESTROY:running=false;PostQuitMessage(0);return 0;
    case WM_ERASEBKGND:return 1;default:return DefWindowProcW(h,m,w,l);}}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show){
    wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);folder=std::filesystem::path(path).parent_path();DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",path,32768);
    if(!n||n>=32768){MessageBoxW(nullptr,L"LOCALAPPDATA is unavailable.",L"Animated prototype",MB_OK|MB_ICONERROR);return 1;}
    savefile=std::filesystem::path(path)/L"HybridNoMercyAnimatedDemo/career.json";std::string error;
    auto legacy=std::filesystem::path(path)/L"HybridNoMercyCareerLab/career.json";
    if(std::filesystem::exists(savefile)){
        if(!game.load(savefile,error)){writable=false;message="Could not read save: "+error;}
        else message="Career restored. E at a marker starts a fight; P opens animation practice.";
    }else if(std::filesystem::exists(legacy)){
        if(!game.load(legacy,error)){writable=false;message="Previous lab save could not be read: "+error;}
        else message="Imported your lab career. Original save retained. P opens animation practice.";
    }
    if(writable&&!game.save(savefile,error)){writable=false;message="Could not initialize save: "+error;}
    WNDCLASSW wc{};wc.style=CS_OWNDC;wc.lpfnWndProc=proc;wc.hInstance=instance;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.lpszClassName=L"HybridAnimatedWorld";RegisterClassW(&wc);
    RECT bounds{0,0,1280,800};AdjustWindowRect(&bounds,WS_OVERLAPPEDWINDOW,FALSE);window=CreateWindowW(wc.lpszClassName,L"Hybrid / No Mercy - Original Animated World Prototype",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nullptr,instance,nullptr);
    dc=GetDC(window);PIXELFORMATDESCRIPTOR pf{};pf.nSize=sizeof(pf);pf.nVersion=1;pf.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pf.iPixelType=PFD_TYPE_RGBA;pf.cColorBits=24;pf.cDepthBits=24;pf.iLayerType=PFD_MAIN_PLANE;
    int format=ChoosePixelFormat(dc,&pf);if(!format||!SetPixelFormat(dc,format,&pf)||(context=wglCreateContext(dc))==nullptr||!wglMakeCurrent(dc,context)){MessageBoxW(window,L"OpenGL graphics initialization failed. Update your GPU driver.",L"Animated prototype",MB_OK|MB_ICONERROR);return 2;}
    auto face=CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE,L"Consolas");auto old=SelectObject(dc,face);font=glGenLists(256);if(!wglUseFontBitmapsA(dc,0,256,font)){MessageBoxW(window,L"Could not create the text font.",L"Animated prototype",MB_OK|MB_ICONERROR);return 3;}SelectObject(dc,old);DeleteObject(face);
    quad=gluNewQuadric();glEnable(GL_DEPTH_TEST);glEnable(GL_NORMALIZE);glEnable(GL_COLOR_MATERIAL);glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);GLfloat ambient[]={.42f,.47f,.50f,1};glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);GLfloat diffuse[]={.82f,.80f,.76f,1};glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);
    ShowWindow(window,show);UpdateWindow(window);music(false);auto last=std::chrono::steady_clock::now();MSG event{};
    while(running){while(PeekMessageW(&event,nullptr,0,0,PM_REMOVE)){TranslateMessage(&event);DispatchMessageW(&event);}if(!running)break;auto now=std::chrono::steady_clock::now();float dt=std::chrono::duration<float>(now-last).count();last=now;tick(std::min(dt,.20f));if(!IsIconic(window))render();Sleep(8);}
    audio.stop();gluDeleteQuadric(quad);glDeleteLists(font,256);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);return 0;
}
