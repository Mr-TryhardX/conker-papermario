#include "ui_career.h"
#include "../career/career.hpp"
#include <functional>

namespace hybridheaven { std::filesystem::path get_app_folder_path(); }
namespace {
career::Game game;
bool initialized=false,writable=true;
std::string name,status,progress,message,perk;
std::vector<std::string> roster,stats,shop,moves,encounters;
std::filesystem::path save_path;
void refresh() {
    const auto& f=game.state.roster[game.state.selected];
    name=career::fighters[game.state.selected].name;
    status="Level "+std::to_string(f.level)+" | XP "+std::to_string(f.xp)+"/"+std::to_string(game.next_xp())+" | Credits "+std::to_string(game.state.coins)+" | Training points "+std::to_string(f.points)+" | Attire "+std::to_string(f.costume+1);
    perk=std::string(career::fighters[game.state.selected].perk)+": "+career::fighters[game.state.selected].perk_detail;
    int cleared=0;for(bool c:game.state.cleared)cleared+=c;
    progress="Lab chapter "+std::to_string(cleared)+"/6 | Wins "+std::to_string(game.state.wins)+" | Losses "+std::to_string(game.state.losses);
    if(game.battle.active)progress+=" | HP "+std::to_string(game.battle.hp)+" | Stamina "+std::to_string(game.battle.stamina)+" | Enemy HP "+std::to_string(game.battle.enemy_hp);
    roster.clear();stats.clear();shop.clear();moves.clear();encounters.clear();
    for(int i=0;i<career::fighter_count;++i)roster.push_back(std::string(career::fighters[i].name)+(game.state.roster[i].unlocked?"":" / unlock "+std::to_string(career::fighters[i].price)));
    for(int i=0;i<career::stat_count;++i)stats.push_back(std::string(career::stat_names[i])+" "+std::to_string(game.stat(i))+" / train +1");
    for(int i=0;i<career::move_count;++i){shop.push_back(std::string(career::moves[i].name)+(f.learned[i]?" / owned":" / "+std::to_string(career::moves[i].price)+" credits / level "+std::to_string(career::moves[i].required_level)));moves.push_back(career::moves[i].name);}
    for(int i=0;i<career::encounter_count;++i)encounters.push_back(std::string(career::encounters[i].name)+(game.state.cleared[i]?" / cleared":""));
}
void act(const std::string& action,int index) {
    if(!writable){message="Career save could not be read. Changes are disabled to protect it.";return;}
    const auto before=game;bool changed=false;
    if(action=="select")changed=game.select(index,message);
    else if(action=="attire")changed=game.select_costume(index,message);
    else if(action=="train")changed=game.upgrade(index,message);
    else if(action=="buy")changed=game.buy_move(index,message);
    else if(action=="medkit")changed=game.buy_medkit(message);
    else if(action=="begin")changed=game.begin(index,message);
    else if(action=="attack")changed=game.attack(index,message);
    else if(action=="guard")changed=game.recover(true,message);
    else if(action=="recover")changed=game.recover(false,message);
    else if(action=="heal")changed=game.heal(message);
    else if(action=="retreat"){changed=game.battle.active;game.retreat(message);}
    if(changed){std::string error;if(!game.save(save_path,error)){game=before;message="Action rolled back because saving failed: "+error;}}
}
}
void recompui::make_career_bindings(Rml::Context* context) {
    if(!initialized){
        save_path=hybridheaven::get_app_folder_path()/"career-lab"/"career.json";
        std::string error;
        if(std::filesystem::exists(save_path)){writable=game.load(save_path,error);message=writable?"Career loaded.":"Career could not be read: "+error;}
        else{writable=game.save(save_path,error);message=writable?"Kane is available. Start Training Floor to earn XP.":"Could not create career save: "+error;}
        initialized=true;
    }
    auto c=context->CreateDataModel("career_model");
    if(!c)throw std::runtime_error("Could not create career data model");
    c.Bind("name",&name);c.Bind("status",&status);c.Bind("progress",&progress);c.Bind("message",&message);c.Bind("perk",&perk);
    c.Bind("roster",&roster);c.Bind("stats",&stats);c.Bind("shop",&shop);c.Bind("moves",&moves);c.Bind("encounters",&encounters);
    c.BindEventCallback("career_action",[](Rml::DataModelHandle handle,Rml::Event&,const Rml::VariantList& args){
        if(args.empty())return;
        act(args[0].Get<std::string>(),args.size()>1?args[1].Get<int>():0);refresh();handle.DirtyAllVariables();
    });
    refresh();
}
