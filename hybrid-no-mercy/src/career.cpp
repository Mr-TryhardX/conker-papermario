#include "career.hpp"
#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace career {
const std::array<const char*,stat_count> stat_names={"Power","Grapple","Technique","Defense","Speed","Endurance"};
const std::array<FighterDef,fighter_count> fighters={{
    {"Johnny Slater","Balanced",0,{7,6,7,7,7,7},"Adaptive Guard","Guard restores 4 health before the opponent attacks."},
    {"Stone Cold","Brawler",0,{10,8,5,8,5,8},"Never Back Down","Strikes deal +6 damage below half health."},
    {"The Rock","Showman",120,{8,9,8,6,7,7},"People's Champion","Earn 20% more credits from victories."},
    {"Kurt Angle","Technician",150,{6,10,11,7,8,6},"Olympic Technique","Technique moves deal +7 damage and cost 2 less stamina."},
    {"The Undertaker","Powerhouse",180,{12,10,5,10,3,9},"Deadman","Once per encounter, survive a fatal hit with 1 health."},
    {"Kane","Powerhouse",0,{12,11,4,9,4,10},"Big Red Machine","Throws deal +5 damage; medkits heal 10 extra health."}
}};
const std::array<MoveDef,move_count> moves={{
    {"Jab",Power,7,3,0,1},{"Kick",Power,11,6,0,1},
    {"Body Slam",Grapple,14,9,0,1},{"Suplex",Grapple,20,12,55,2},
    {"DDT",Technique,22,13,75,2},{"Submission",Technique,27,15,100,3},
    {"Powerbomb",Grapple,33,18,135,4},{"Finisher",Power,42,23,180,5}
}};
// Original encounters used to exercise the RPG systems. These are not imported
// Hybrid Heaven rooms or WWF match logic; the UI explicitly labels this lab.
const std::array<EncounterDef,encounter_count> encounters={{
    {"Training Floor",40,9,2,60,50},{"Security Checkpoint",62,12,3,85,65},
    {"Maintenance Wing",82,14,4,105,80},{"Containment Unit",108,16,5,130,100},
    {"Service Lift",130,19,6,155,120},{"Facility Champion",165,22,7,200,160}
}};
State::State() { roster[0].unlocked=true; roster[1].unlocked=true; roster[5].unlocked=true; selected=5; }
int Game::stat(int i) const { return fighters.at(state.selected).base.at(i)+state.roster.at(state.selected).training.at(i); }
int Game::max_hp() const { return 70+stat(Endurance)*6+state.roster.at(state.selected).level*5; }
int Game::max_stamina() const { return 26+stat(Endurance)*2+state.roster.at(state.selected).level*2; }
int Game::next_xp() const { return 40+state.roster.at(state.selected).level*20; }
bool Game::select(int i,std::string& m) {
    if(battle.active) {m="Finish or retreat from the encounter first.";return false;}
    if(i<0 || i>=fighter_count) {m="Unknown fighter.";return false;}
    if(!state.roster[i].unlocked) {
        if(state.coins<fighters[i].price) {m="Not enough credits to unlock this profile.";return false;}
        state.coins-=fighters[i].price;state.roster[i].unlocked=true;
    }
    state.selected=i;m=std::string("Selected ")+fighters[i].name+". This changes the lab profile; 3D model conversion is pending.";return true;
}
bool Game::upgrade(int i,std::string& m) {
    if(battle.active) {m="Training is available between encounters.";return false;}
    if(i<0 || i>=stat_count) {m="Unknown stat.";return false;}
    auto& f=state.roster[state.selected];
    if(f.points<=0) {m="Earn training points by leveling up.";return false;}
    if(f.training[i]>=20) {m="This stat has reached its training cap.";return false;}
    --f.points;++f.training[i];m=std::string(stat_names[i])+" increased.";return true;
}
bool Game::select_costume(int i,std::string& m) {
    if(battle.active) {m="Change attire between encounters.";return false;}
    if(i<0 || i>=costume_count) {m="Unknown costume slot.";return false;}
    state.roster[state.selected].costume=i;
    m="Selected attire slot "+std::to_string(i+1)+". Model and texture import are pending.";return true;
}
bool Game::buy_move(int i,std::string& m) {
    if(battle.active) {m="The shop is available between encounters.";return false;}
    if(i<0 || i>=move_count) {m="Unknown move.";return false;}
    auto& f=state.roster[state.selected];const auto& move=moves[i];
    if(f.learned[i]) {m="This fighter already knows that move.";return false;}
    if(f.level<move.required_level) {m="Reach level "+std::to_string(move.required_level)+" first.";return false;}
    if(state.coins<move.price) {m="Not enough credits.";return false;}
    state.coins-=move.price;f.learned[i]=true;m=std::string("Learned ")+move.name+".";return true;
}
bool Game::buy_medkit(std::string& m) {
    if(battle.active) {m="The shop is available between encounters.";return false;}
    if(state.medkits>=10) {m="Medkit inventory is full (10).";return false;}
    if(state.coins<25) {m="A medkit costs 25 credits.";return false;}
    state.coins-=25;++state.medkits;m="Bought a medkit.";return true;
}
bool Game::begin(int i,std::string& m) {
    if(battle.active) {m="An encounter is already running.";return false;}
    if(i<0 || i>=encounter_count || (i>0 && !state.cleared[i-1])) {m="Clear the preceding encounter first.";return false;}
    battle=Battle{};battle.active=true;battle.encounter=i;battle.hp=max_hp();battle.stamina=max_stamina();
    battle.enemy_hp=encounters[i].health;battle.log.push_back(std::string("Entered ")+encounters[i].name+".");
    m="Choose a move, guard, or recover stamina.";return true;
}
bool Game::attack(int i,std::string& m) {
    if(!battle.active) {m="Choose an encounter first.";return false;}
    if(i<0 || i>=move_count || !state.roster[state.selected].learned[i]) {m="This fighter has not learned that move.";return false;}
    const auto& move=moves[i];
    const int cost=move.stamina-(state.selected==3 && move.stat==Technique?2:0);
    if(battle.stamina<cost) {m="Not enough stamina. Guard or recover.";return false;}
    battle.stamina-=cost;
    int bonus=0;
    if(state.selected==1 && move.stat==Power && battle.hp*2<max_hp())bonus=6;
    if(state.selected==3 && move.stat==Technique)bonus=7;
    if(state.selected==5 && move.stat==Grapple)bonus=5;
    const int damage=std::max(1,move.power+stat(move.stat)+bonus-encounters[battle.encounter].defense);
    battle.enemy_hp=std::max(0,battle.enemy_hp-damage);
    battle.log.push_back(std::string(move.name)+": "+std::to_string(damage)+" damage.");
    if(!battle.enemy_hp) victory();else enemy_turn(false);
    m=battle.log.back();return true;
}
bool Game::recover(bool guard,std::string& m) {
    if(!battle.active) {m="Choose an encounter first.";return false;}
    battle.stamina=std::min(max_stamina(),battle.stamina+(guard?9:18));
    if(guard && state.selected==0)battle.hp=std::min(max_hp(),battle.hp+4);
    battle.log.push_back(guard?"Guarded and regained 9 stamina.":"Recovered 18 stamina.");enemy_turn(guard);
    m=battle.log.back();return true;
}
bool Game::heal(std::string& m) {
    if(!battle.active) {m="Health is restored at the start of each lab encounter.";return false;}
    if(!state.medkits) {m="No medkits left.";return false;}
    if(battle.hp==max_hp()) {m="Health is already full.";return false;}
    const int amount=state.selected==5?60:50;
    --state.medkits;battle.hp=std::min(max_hp(),battle.hp+amount);battle.log.push_back("Used a medkit (+"+std::to_string(amount)+" health).");enemy_turn(false);m=battle.log.back();return true;
}
void Game::retreat(std::string& m) {
    if(!battle.active) {m="No encounter is running.";return;}
    battle.active=false;m="Retreated. No rewards earned.";battle.log.push_back(m);
}
void Game::enemy_turn(bool guard) {
    ++battle.turn;
    int damage=std::max(1,encounters[battle.encounter].attack+(battle.turn%3==0?5:0)-stat(Defense)/2);
    if(guard) damage=std::max(1,damage/2);
    if(state.selected==4 && battle.hp<=damage && !battle.perk_used){battle.hp=1;battle.perk_used=true;battle.log.push_back("Deadman: survived a fatal hit!");}
    else battle.hp=std::max(0,battle.hp-damage);
    battle.stamina=std::min(max_stamina(),battle.stamina+2+stat(Speed)/5);
    battle.log.push_back("Opponent hit for "+std::to_string(damage)+".");
    if(!battle.hp) {++state.losses;battle.active=false;battle.log.push_back("Defeat. Keep your upgrades; retry from the career menu.");}
}
void Game::victory() {
    const auto& e=encounters[battle.encounter];auto& f=state.roster[state.selected];
    const bool replay=state.cleared[battle.encounter];const int xp=replay?e.xp/3:e.xp;int credits=replay?e.coins/3:e.coins;
    if(state.selected==2)credits=credits*120/100;
    state.coins=std::min(1000000,state.coins+credits);f.xp+=xp;++state.wins;state.cleared[battle.encounter]=true;battle.active=false;
    battle.log.push_back("Victory! +"+std::to_string(xp)+" XP, +"+std::to_string(credits)+" credits.");
    while(f.level<50 && f.xp>=40+f.level*20) {f.xp-=40+f.level*20;++f.level;f.points+=3;battle.log.push_back("Level "+std::to_string(f.level)+"! +3 training points.");}
    if(f.level==50) f.xp=std::min(f.xp,1039);
}
nlohmann::json Game::serialize() const {
    nlohmann::json data={{"version",state.version},{"selected",state.selected},{"coins",state.coins},{"medkits",state.medkits},{"wins",state.wins},{"losses",state.losses},{"cleared",state.cleared}};
    data["roster"]=nlohmann::json::array();
    for(const auto& f:state.roster) data["roster"].push_back({{"unlocked",f.unlocked},{"level",f.level},{"xp",f.xp},{"points",f.points},{"costume",f.costume},{"training",f.training},{"learned",f.learned}});
    return data;
}
bool Game::restore(const nlohmann::json& input,std::string& error) {
    try {
        State next;
        auto bounded=[](const nlohmann::json& j,int low,int high) {if(!j.is_number_integer())throw std::runtime_error("Expected an integer.");auto n=j.get<int64_t>();if(n<low || n>high)throw std::runtime_error("Save field is out of range.");return static_cast<int>(n);};
        auto d=input;
        const int version=bounded(d.at("version"),1,2);
        if(version==1) {
            if(d.at("roster").is_array() && d.at("roster").size()==5)d["roster"].push_back(Game{}.serialize()["roster"][5]);
            for(auto& row:d.at("roster"))if(!row.contains("costume"))row["costume"]=0;
            d["version"]=2;
        }
        next.version=2;next.selected=bounded(d.at("selected"),0,fighter_count-1);
        next.coins=bounded(d.at("coins"),0,1000000);next.medkits=bounded(d.at("medkits"),0,10);
        next.wins=bounded(d.at("wins"),0,1000000);next.losses=bounded(d.at("losses"),0,1000000);
        const auto& cleared=d.at("cleared");if(cleared.size()!=encounter_count || !cleared.is_array())throw std::runtime_error("Invalid encounter array.");
        for(int i=0;i<encounter_count;++i) {if(!cleared[i].is_boolean())throw std::runtime_error("Invalid progress flag.");next.cleared[i]=cleared[i].get<bool>();if(i && next.cleared[i] && !next.cleared[i-1])throw std::runtime_error("Progress order is invalid.");}
        if(!d.at("roster").is_array() || d.at("roster").size()!=fighter_count)throw std::runtime_error("Invalid roster.");
        for(int i=0;i<fighter_count;++i) {
            const auto& j=d.at("roster")[i];auto& f=next.roster[i];
            if(!j.at("unlocked").is_boolean())throw std::runtime_error("Invalid unlock flag.");
            f.unlocked=j.at("unlocked").get<bool>();
            f.level=bounded(j.at("level"),1,50);f.xp=bounded(j.at("xp"),0,39+f.level*20);f.points=bounded(j.at("points"),0,147);f.costume=bounded(j.at("costume"),0,costume_count-1);
            if(!j.at("training").is_array() || j.at("training").size()!=stat_count || !j.at("learned").is_array() || j.at("learned").size()!=move_count)throw std::runtime_error("Invalid fighter arrays.");
            for(int k=0;k<stat_count;++k)f.training[k]=bounded(j.at("training")[k],0,20);
            int spent=0;for(int n:f.training)spent+=n;if(spent+f.points>(f.level-1)*3)throw std::runtime_error("Training points exceed earned points.");
            for(int k=0;k<move_count;++k){if(!j.at("learned")[k].is_boolean())throw std::runtime_error("Invalid move flag.");f.learned[k]=j.at("learned")[k].get<bool>();if(f.learned[k] && f.level<moves[k].required_level)throw std::runtime_error("Move requires a higher level.");}
            if(!f.learned[0] || !f.learned[1] || !f.learned[2])throw std::runtime_error("Starting moves are missing.");
        }
        if(!next.roster[0].unlocked || !next.roster[1].unlocked || !next.roster[5].unlocked || !next.roster[next.selected].unlocked)throw std::runtime_error("Selected fighter is locked.");
        state=next;battle=Battle{};error.clear();return true;
    } catch(const std::exception& e) {error=e.what();return false;}
}
static uint32_t checksum(const std::string& s) {uint32_t n=2166136261U;for(unsigned char c:s){n^=c;n*=16777619U;}return n;}
bool Game::save(const std::filesystem::path& path,std::string& error) const {
    try {
        const auto data=serialize();const nlohmann::json envelope={{"format","hybrid-no-mercy-career"},{"checksum",checksum(data.dump())},{"data",data}};
        if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
        auto tmp=path;tmp += ".tmp";auto backup=path;backup += ".bak";
        {std::ofstream out(tmp,std::ios::binary|std::ios::trunc);out.exceptions(std::ios::badbit|std::ios::failbit);out<<envelope.dump(2);out.flush();}
        if(std::filesystem::exists(path))std::filesystem::copy_file(path,backup,std::filesystem::copy_options::overwrite_existing);
#ifdef _WIN32
        // The Win32 frontend replaces the destination atomically via its helper.
        extern bool replace_save_file(const std::filesystem::path&,const std::filesystem::path&);
        if(!replace_save_file(tmp,path))throw std::runtime_error("Could not replace the save file.");
#else
        std::filesystem::rename(tmp,path);
#endif
        error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
bool Game::load(const std::filesystem::path& path,std::string& error) {
    try {
        if(std::filesystem::file_size(path)>65536)throw std::runtime_error("Save is unexpectedly large.");
        std::ifstream in(path,std::ios::binary);in.exceptions(std::ios::badbit|std::ios::failbit);nlohmann::json j;in>>j;
        if(j.at("format")!="hybrid-no-mercy-career" || j.at("checksum").get<uint32_t>()!=checksum(j.at("data").dump()))throw std::runtime_error("Save checksum mismatch.");
        return restore(j.at("data"),error);
    }catch(const std::exception& e){error=e.what();return false;}
}
}
