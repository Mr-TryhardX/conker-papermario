#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <mmsystem.h>
#include "career.hpp"
#include "wave_music.hpp"
#include <functional>
#include <sstream>
#include <random>

namespace career {
bool replace_save_file(const std::filesystem::path& from,const std::filesystem::path& to) {
    return MoveFileExW(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
}
}
namespace {
career::Game game;
std::filesystem::path save_path, app_path;
std::string message="Welcome. Clear Training Floor to earn your first level and training points.";
bool writable=true,music=true;
std::string track="Shop music";
std::mt19937 random_engine(std::random_device{}());
WaveMusic audio;
int page=0,focus=0;
HWND window;
HFONT normal_font,title_font,small_font;
struct Button { RECT rect; int id; bool enabled; };
std::vector<Button> buttons;
const COLORREF bg=RGB(11,14,12),panel=RGB(24,31,25),green=RGB(148,219,83),text=RGB(231,235,225),dim=RGB(160,174,154);
std::wstring wide(const std::string& s) {
    if(s.empty())return L"";
    int n=MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),nullptr,0);
    std::wstring out(n,L' ');MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),out.data(),n);return out;
}
void box(HDC dc,RECT r,COLORREF color) {auto b=CreateSolidBrush(color);FillRect(dc,&r,b);DeleteObject(b);}
void label(HDC dc,const std::string& s,int x,int y,int width,int height=30,COLORREF color=text,HFONT font=nullptr) {
    RECT r{x,y,x+width,y+height};SelectObject(dc,font?font:normal_font);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);
    auto w=wide(s);DrawTextW(dc,w.c_str(),static_cast<int>(w.size()),&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
}
void button(HDC dc,int id,const std::string& s,int x,int y,int width=220,bool enabled=true) {
    RECT r{x,y,x+width,y+42};int index=static_cast<int>(buttons.size());buttons.push_back({r,id,enabled});
    box(dc,r,index==focus?RGB(62,81,46):RGB(34,43,35));
    if(index==focus){auto pen=CreatePen(PS_SOLID,2,green);auto old=SelectObject(dc,pen);auto brush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));Rectangle(dc,r.left,r.top,r.right,r.bottom);SelectObject(dc,brush);SelectObject(dc,old);DeleteObject(pen);}
    label(dc,s,x+12,y+10,width-20,28,enabled?text:RGB(102,116,98));
}
void bar(HDC dc,int value,int maximum,int x,int y,int width,COLORREF color) {box(dc,{x,y,x+width,y+14},RGB(45,50,44));box(dc,{x,y,x+width*value/std::max(1,maximum),y+14},color);}
void play_music() {
    auto path=app_path/L"assets"/L"audio"/L"no-mercy-shop.wav";
    track="Shop music";
    if(game.battle.active) {
        std::vector<std::filesystem::path> themes;
        auto folder=app_path/L"assets"/L"audio"/L"entrances";
        if(std::filesystem::exists(folder))for(const auto& entry:std::filesystem::directory_iterator(folder))if(entry.is_regular_file() && entry.path().extension()==L".wav")themes.push_back(entry.path());
        if(!themes.empty()) {std::uniform_int_distribution<size_t> choose(0,themes.size()-1);path=themes[choose(random_engine)];track=path.stem().string();}
        else {track="Entrance themes pending extraction";path.clear();}
    }
    if(music && !path.empty() && std::filesystem::exists(path)){
        std::string error;
        if(!audio.play(path,error)){track="Audio playback failed";message="Music error: "+error;}
    }else{
        audio.stop();
        if(music){track="Music files missing";message="Extract the whole ZIP and keep assets beside CareerLab.exe.";}
    }
}
void persist_or_rollback(const career::Game& before) {
    std::string error;
    if(!game.save(save_path,error)){game=before;message="Save failed; action rolled back: "+error;}
}
void activate(int id) {
    if(id>=0 && id<6){if(game.battle.active && id!=4){message="Finish or retreat from the encounter first.";return;}page=id;focus=0;InvalidateRect(window,nullptr,FALSE);return;}
    if(id==900){music=!music;play_music();InvalidateRect(window,nullptr,FALSE);return;}
    if(id==901){auto folder=save_path.parent_path();ShellExecuteW(window,L"open",folder.c_str(),nullptr,nullptr,SW_SHOWNORMAL);return;}
    if(!writable){message="The existing save could not be read. Back it up or repair it before making changes.";InvalidateRect(window,nullptr,FALSE);return;}
    auto before=game;bool changed=false;
    if(id>=100 && id<100+career::fighter_count)changed=game.select(id-100,message);
    if(id>=160 && id<160+career::costume_count)changed=game.select_costume(id-160,message);
    if(id>=200 && id<206)changed=game.upgrade(id-200,message);
    if(id>=300 && id<308)changed=game.buy_move(id-300,message);
    if(id==350)changed=game.buy_medkit(message);
    if(id>=400 && id<406){changed=game.begin(id-400,message);if(changed)page=4;}
    if(id>=500 && id<508)changed=game.attack(id-500,message);
    if(id==550)changed=game.recover(true,message);
    if(id==551)changed=game.recover(false,message);
    if(id==552)changed=game.heal(message);
    if(id==553){changed=game.battle.active;game.retreat(message);}
    if(changed)persist_or_rollback(before);
    if(before.battle.active!=game.battle.active)play_music();
    InvalidateRect(window,nullptr,FALSE);
}
void render(HDC dc) {
    RECT client;GetClientRect(window,&client);box(dc,client,bg);buttons.clear();
    label(dc,"HYBRID / NO MERCY",28,20,950,48,green,title_font);
    label(dc,"CAREER SYSTEMS LAB  |  Original prototype encounters",30,69,1030,28,dim,small_font);
    label(dc,"3D game integration, wrestler models and imported move animations are still pending.",30,94,1060,26,dim,small_font);
    const char* tabs[]={"Career","Roster","Training","Shop","Encounter","Records"};
    for(int i=0;i<6;++i)button(dc,i,tabs[i],28+i*173,132,165,!game.battle.active || i==4);
    box(dc,{28,190,300,650},panel);
    const auto& f=game.state.roster[game.state.selected];
    label(dc,career::fighters[game.state.selected].name,44,208,242,40,green);
    label(dc,std::string(career::fighters[game.state.selected].style)+" / Attire "+std::to_string(f.costume+1),44,249,240,28,dim);
    label(dc,"Level "+std::to_string(f.level)+"   XP "+std::to_string(f.xp)+" / "+std::to_string(game.next_xp()),44,287,245);
    bar(dc,f.xp,game.next_xp(),44,323,238,green);
    label(dc,"Credits: "+std::to_string(game.state.coins),44,353,240);
    label(dc,"Training points: "+std::to_string(f.points),44,389,240);
    label(dc,"Medkits: "+std::to_string(game.state.medkits)+" / 10",44,425,240);
    for(int i=0;i<career::stat_count;++i)label(dc,std::string(career::stat_names[i])+": "+std::to_string(game.stat(i)),44,472+i*25,240,25,text,small_font);
    int x=328,y=203;
    if(page==0){
        label(dc,"FACILITY BREACH — TEST CHAPTER",x,y,730,40,green);
        label(dc,"Six encounters test the career loop. First clears earn full rewards; replays earn one third. Each fighter keeps separate levels, training and learned moves.",x,y+45,700,80,dim);
        for(int i=0;i<career::encounter_count;++i){bool unlocked=i==0 || game.state.cleared[i-1];std::string s=std::to_string(i+1)+". "+career::encounters[i].name+(game.state.cleared[i]?" [cleared]":unlocked?"":" [locked]");button(dc,400+i,s,x,y+142+i*48,712,unlocked && !game.battle.active);}
    }else if(page==1){
        label(dc,"SELECT A FIGHTER PROFILE",x,y,730,40,green);
        label(dc,"Profiles affect lab combat stats. They do not yet replace the character model in Hybrid Heaven.",x,y+48,710,62,dim);
        for(int i=0;i<career::fighter_count;++i){const auto& p=career::fighters[i];const auto& c=game.state.roster[i];std::string s=std::string(p.name)+" / "+p.style+" / "+(c.unlocked?"Level "+std::to_string(c.level):std::to_string(p.price)+" credits");button(dc,100+i,s,x,y+105+i*46,712,!game.battle.active);}
        for(int i=0;i<career::costume_count;++i)button(dc,160+i,"Attire "+std::to_string(i+1)+(f.costume==i?" [selected]":""),x+i*180,y+390,170,!game.battle.active);
    }else if(page==2){
        label(dc,"TRAINING",x,y,730,40,green);
        label(dc,"Each level grants 3 points. Spend one point for +1 to a stat; each stat supports 20 training upgrades.",x,y+48,710,64,dim);
        for(int i=0;i<career::stat_count;++i)button(dc,200+i,std::string(career::stat_names[i])+": "+std::to_string(game.stat(i))+"  [+1]",x+(i%2)*362,y+136+(i/2)*65,350,f.points>0 && f.training[i]<20 && !game.battle.active);
        label(dc,"Power: strikes / finishers\nGrapple: throws\nTechnique: DDT / submissions\nDefense: reduces incoming damage\nEndurance: health and stamina\nSpeed: stamina regeneration",x,y+350,710,175,dim,small_font);
    }else if(page==3){
        label(dc,"SMACKDOWN-STYLE TRAINING SHOP",x,y,730,40,green);
        label(dc,"Move purchases belong to the selected fighter. Credits and medkits are shared across the roster.",x,y+48,710,63,dim);
        for(int i=3;i<career::move_count;++i){const auto& move=career::moves[i];std::string s=std::string(move.name)+" / Lv "+std::to_string(move.required_level)+" / "+(f.learned[i]?"owned":std::to_string(move.price)+" credits");button(dc,300+i,s,x,y+126+(i-3)*51,712,!f.learned[i] && f.level>=move.required_level && !game.battle.active);}
        button(dc,350,"Medkit / 25 credits / heals "+std::to_string(game.state.selected==5?60:50)+" HP",x,y+390,712,game.state.medkits<10 && !game.battle.active);
    }else if(page==4){
        if(!game.battle.active){label(dc,"ENCOUNTER RESULTS",x,y,730,40,green);label(dc,"Choose Career to start or replay an encounter.",x,y+44,710,50,dim);for(size_t i=0;i<game.battle.log.size() && i<10;++i)label(dc,game.battle.log[game.battle.log.size()-std::min<size_t>(10,game.battle.log.size())+i],x,y+105+static_cast<int>(i)*30,710,30,text,small_font);}
        else {
            label(dc,career::encounters[game.battle.encounter].name,x,y,710,35,green);
            label(dc,"HP "+std::to_string(game.battle.hp)+" / "+std::to_string(game.max_hp())+"     Stamina "+std::to_string(game.battle.stamina)+" / "+std::to_string(game.max_stamina()),x,y+40,710);
            bar(dc,game.battle.hp,game.max_hp(),x,y+76,340,green);bar(dc,game.battle.stamina,game.max_stamina(),x+372,y+76,340,RGB(100,173,225));
            label(dc,"Opponent HP: "+std::to_string(game.battle.enemy_hp),x,y+96,710);
            for(int i=0;i<career::move_count;++i)button(dc,500+i,std::string(career::moves[i].name)+" / "+std::to_string(career::moves[i].stamina)+" ST",x+(i%2)*362,y+135+(i/2)*48,350,f.learned[i] && game.battle.stamina>=career::moves[i].stamina);
            button(dc,550,"Guard (+9 ST)",x,y+340,170);button(dc,551,"Recover (+18 ST)",x+180,y+340,175);button(dc,552,"Use medkit",x+365,y+340,165,game.state.medkits>0);button(dc,553,"Retreat",x+540,y+340,170);
            size_t count=std::min<size_t>(2,game.battle.log.size());for(size_t i=0;i<count;++i)label(dc,game.battle.log[game.battle.log.size()-count+i],x,y+390+static_cast<int>(i)*28,710,28,dim,small_font);
        }
    }else {
        int cleared=0;for(bool c:game.state.cleared)cleared+=c;
        label(dc,"PROGRESSION RECORD",x,y,730,40,green);
        label(dc,"Test chapter: "+std::to_string(cleared)+" / 6 encounters cleared\nWins: "+std::to_string(game.state.wins)+"    Losses: "+std::to_string(game.state.losses),x,y+55,710,80);
        label(dc,"This is prototype chapter progress, not a percentage of the combined games. Career saves are separate from Hybrid Heaven and No Mercy saves.",x,y+155,710,80,dim);
        label(dc,"Autosave: after successful purchases, upgrades, selections and combat actions. Closing during an encounter returns you to the career menu on restart.",x,y+260,710,85,dim);
        button(dc,901,"Open career save folder",x,y+372,712);
    }
    label(dc,std::string("Perk: ")+career::fighters[game.state.selected].perk,328,648,710,24,green,small_font);
    box(dc,{28,672,1064,740},panel);label(dc,message,42,684,1002,48,text,small_font);
    button(dc,900,music?"Music: ON":"Music: OFF",28,756,195);
    label(dc,"Mouse / arrows / Enter: select   |   "+track,242,767,822,28,dim,small_font);
    if(focus>=static_cast<int>(buttons.size()))focus=0;
}
LRESULT CALLBACK proc(HWND h,UINT event,WPARAM w,LPARAM l) {
    switch(event){
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);HDC mem=CreateCompatibleDC(dc);auto bitmap=CreateCompatibleBitmap(dc,r.right,r.bottom);auto old=SelectObject(mem,bitmap);render(mem);BitBlt(dc,0,0,r.right,r.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bitmap);DeleteDC(mem);EndPaint(h,&ps);return 0;}
    case WM_ERASEBKGND:return 1;
    case WM_LBUTTONDOWN:{POINT p{static_cast<short>(LOWORD(l)),static_cast<short>(HIWORD(l))};for(size_t i=0;i<buttons.size();++i)if(PtInRect(&buttons[i].rect,p)){focus=static_cast<int>(i);if(buttons[i].enabled)activate(buttons[i].id);else{message="That option is not available yet.";InvalidateRect(h,nullptr,FALSE);}break;}return 0;}
    case WM_KEYDOWN:
        if(w==VK_ESCAPE){activate(0);return 0;}
        if(!buttons.empty() && (w==VK_DOWN || w==VK_RIGHT || w==VK_TAB || w==VK_UP || w==VK_LEFT)){int step=(w==VK_UP || w==VK_LEFT)?-1:1;focus=(focus+step+static_cast<int>(buttons.size()))%static_cast<int>(buttons.size());InvalidateRect(h,nullptr,FALSE);return 0;}
        if((w==VK_RETURN || w==VK_SPACE) && focus>=0 && focus<static_cast<int>(buttons.size()) && buttons[focus].enabled){activate(buttons[focus].id);return 0;}break;
    case WM_DESTROY:audio.stop();PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(h,event,w,l);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show) {
    SetProcessDPIAware();wchar_t executable[32768];GetModuleFileNameW(nullptr,executable,32768);app_path=std::filesystem::path(executable).parent_path();
    wchar_t local[32768];DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
    save_path=(n && n<32768?std::filesystem::path(local):app_path)/L"HybridNoMercyCareerLab"/L"career.json";
    std::string error;
    if(std::filesystem::exists(save_path)){if(game.load(save_path,error))message="Career loaded. Welcome back.";else{writable=false;message="Save could not be loaded; changes are disabled: "+error;}}
    else if(!game.save(save_path,error)){writable=false;message="Could not create the career save: "+error;}
    normal_font=CreateFontW(-19,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    small_font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    title_font=CreateFontW(-36,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Arial");
    WNDCLASSW cls{};cls.lpfnWndProc=proc;cls.hInstance=instance;cls.lpszClassName=L"HybridCareerLab";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&cls);
    RECT rect{0,0,1092,815};AdjustWindowRect(&rect,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);
    window=CreateWindowW(cls.lpszClassName,L"Hybrid / No Mercy — Career Systems Lab",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,nullptr);
    if(!window)return 1;
    ShowWindow(window,show);UpdateWindow(window);play_music();
    MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    DeleteObject(normal_font);DeleteObject(small_font);DeleteObject(title_font);return 0;
}
