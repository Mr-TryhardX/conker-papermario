#include "career.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace career;
static int checks=0;
void check(bool condition,const char* detail){++checks;if(!condition)throw std::runtime_error(detail);}
void win(Game& g,int encounter) {
    std::string m;check(g.begin(encounter,m),"Encounter should start");
    for(int turn=0;turn<100 && g.battle.active;++turn) {
        if(g.battle.hp<40 && g.state.medkits){check(g.heal(m),"Healing should work");continue;}
        int best=-1,value=-1;
        for(int i=0;i<move_count;++i)if(g.state.roster[g.state.selected].learned[i] && g.battle.stamina>=moves[i].stamina){int damage=moves[i].power+g.stat(moves[i].stat);if(damage>value){value=damage;best=i;}}
        if(best>=0)check(g.attack(best,m),"Attack should work");else check(g.recover(true,m),"Guard should work");
    }
    check(!g.battle.active && g.battle.enemy_hp==0,"Balanced chapter should be beatable");
}
int main(int argc,char** argv) {
 try {
    std::string m;Game g;
    check(g.state.selected==5 && g.state.roster[5].unlocked,"Kane should be the default free profile");
    for(int i=0;i<costume_count;++i)check(g.select_costume(i,m) && g.state.roster[5].costume==i,"All four attire slots should be selectable");
    check(!g.select_costume(costume_count,m),"Invalid attire slot should be rejected");
    check(g.select(0,m),"Johnny profile should be selectable");
    check(!g.begin(1,m),"Cannot skip locked encounters");
    auto before=g.serialize();check(!g.upgrade(Power,m),"Cannot spend nonexistent points");check(g.serialize()==before,"Rejected training must not mutate state");
    check(!g.buy_move(7,m),"Cannot buy a high-level move early");
    win(g,0);check(g.state.roster[0].level==2,"First clear should level up");check(g.state.roster[0].points==3,"Level should grant points");
    check(g.state.coins==150 && g.state.cleared[0],"First-clear rewards should persist");
    check(g.upgrade(Defense,m) && g.upgrade(Power,m) && g.upgrade(Endurance,m),"Training points should be spendable");
    check(!g.upgrade(Power,m),"Overspending training points should fail");
    check(g.buy_move(3,m),"Suplex should unlock at level two");int coins=g.state.coins;
    check(!g.buy_move(3,m) && g.state.coins==coins,"Buying an owned move must not deduct credits");
    auto johnny=g.state.roster[0];check(g.select(1,m),"Free fighter should be selectable");
    check(g.state.roster[1].level==1,"Each fighter needs separate XP");
    check(g.select(5,m) && g.state.roster[5].costume==3,"Kane attire should survive character switching");
    check(g.select(0,m) && g.state.roster[0].level==johnny.level,"Switching back must preserve progress");
    check(g.begin(0,m),"Replay should start");before=g.serialize();
    check(!g.select(1,m) && !g.buy_medkit(m) && !g.upgrade(Defense,m) && !g.select_costume(1,m),"Roster, shop, training and costumes must be blocked during battle");
    check(g.serialize()==before,"Blocked battle actions must not mutate career");g.retreat(m);
    coins=g.state.coins;win(g,0);check(g.state.coins-coins==50/3,"Replay should grant reduced credits");
    for(int e=1;e<encounter_count;++e) {
        auto& f=g.state.roster[0];
        while(f.points>0)check(g.upgrade(f.points%3==0?Defense:f.points%3==1?Endurance:Power,m),"Chapter training should work");
        for(int i=3;i<move_count;++i)if(!f.learned[i] && f.level>=moves[i].required_level && g.state.coins>=moves[i].price)g.buy_move(i,m);
        if(g.state.medkits<2 && g.state.coins>=25)g.buy_medkit(m);
        win(g,e);
    }
    check(g.state.cleared.back(),"Full test chapter should complete");
    check(g.select(2,m),"Earned credits should unlock a wrestler profile");
    check(g.state.roster[2].unlocked && g.state.roster[2].level==1,"New profile should start at level one");
    Game loaded;check(loaded.restore(g.serialize(),m),"Valid career should deserialize");check(loaded.serialize()==g.serialize(),"Roundtrip should preserve all progression");
    auto invalid=g.serialize();invalid["roster"][2]["points"]=-1;before=loaded.serialize();check(!loaded.restore(invalid,m),"Negative points should be rejected");check(loaded.serialize()==before,"Rejected save must preserve loaded career");
    invalid=g.serialize();invalid["version"]=3;check(!loaded.restore(invalid,m),"Unknown save version should be rejected");
    auto legacy=g.serialize();legacy["version"]=1;legacy["roster"].erase(5);legacy["selected"]=0;for(auto& row:legacy["roster"])row.erase("costume");
    check(loaded.restore(legacy,m) && loaded.state.version==2 && loaded.state.roster[5].unlocked && loaded.state.roster[0].level==g.state.roster[0].level,"Legacy career should migrate without losing progression");
    invalid=g.serialize();invalid["coins"]=1e30;check(!loaded.restore(invalid,m),"Overflowing credits should be rejected");
    invalid=g.serialize();invalid["roster"][2]["level"]=1;invalid["roster"][2]["training"][0]=20;check(!loaded.restore(invalid,m),"Unfunded upgrades should be rejected");
    auto path=std::filesystem::path(argc>1?argv[1]:"/tmp/hybrid-career-test.json");
    check(g.save(path,m),"First save should succeed");check(loaded.load(path,m),"Save should load");
    check(g.select(0,m) && g.save(path,m),"Replacing a save should work");check(std::filesystem::exists(path.string()+".bak"),"Previous save should be backed up");
    check(loaded.load(path,m) && loaded.serialize()==g.serialize(),"Replaced save should reload accurately");
    {std::ofstream out(path);out<<"{broken";}before=loaded.serialize();check(!loaded.load(path,m),"Corrupt save should fail");check(before==loaded.serialize(),"Corrupt save must not destroy in-memory career");
    check(!g.save(path/"invalid.json",m),"Writing under a file should fail");
    Game failed;check(failed.begin(5,m)==false,"Final encounter needs progression");
    check(failed.begin(0,m),"Training encounter should start");while(failed.battle.active)failed.recover(false,m);
    check(failed.state.losses==1 && failed.state.wins==0 && !failed.state.cleared[0],"Defeat must not grant completion or rewards");
    check(failed.begin(0,m),"Defeated encounter should be retryable");
    auto perk_game=[](int fighter) {
        Game p;p.state.selected=fighter;p.state.roster[fighter].unlocked=true;
        p.state.roster[fighter].level=10;
        for(int i=0;i<stat_count;++i)p.state.roster[fighter].training[i]=12-fighters[fighter].base[i];
        return p;
    };
    auto regular=perk_game(0),kane=perk_game(5);
    regular.begin(0,m);kane.begin(0,m);regular.attack(2,m);kane.attack(2,m);
    check(regular.battle.enemy_hp-kane.battle.enemy_hp==5,"Kane's throw perk should add damage");
    regular.battle.hp=30;kane.battle.hp=30;regular.heal(m);kane.heal(m);
    check(kane.battle.hp-regular.battle.hp==10,"Kane's medkit perk should add healing");
    auto cold_low=perk_game(1),cold_high=perk_game(1);
    cold_low.begin(0,m);cold_high.begin(0,m);cold_low.battle.hp=30;cold_low.attack(0,m);cold_high.attack(0,m);
    check(cold_high.battle.enemy_hp-cold_low.battle.enemy_hp==6,"Stone Cold perk should trigger below half health");
    auto angle=perk_game(3),neutral=perk_game(0);
    angle.state.roster[3].learned[4]=true;neutral.state.roster[0].learned[4]=true;
    angle.begin(1,m);neutral.begin(1,m); // unlock the second encounter explicitly for this isolated perk test
    angle.state.cleared[0]=true;neutral.state.cleared[0]=true;angle.begin(1,m);neutral.begin(1,m);
    angle.attack(4,m);neutral.attack(4,m);
    check(neutral.battle.enemy_hp-angle.battle.enemy_hp==7 && angle.battle.stamina-neutral.battle.stamina==2,"Angle perk should increase technique damage and reduce cost");
    auto deadman=perk_game(4);deadman.begin(0,m);deadman.battle.hp=1;deadman.recover(false,m);
    check(deadman.battle.active && deadman.battle.hp==1 && deadman.battle.perk_used,"Undertaker should survive one fatal hit");
    deadman.recover(false,m);check(!deadman.battle.active && deadman.state.losses==1,"Undertaker perk must trigger only once");
    auto rock=perk_game(2);int wallet=rock.state.coins;win(rock,0);
    check(rock.state.coins-wallet==60,"Rock's victory perk should increase credits");
    auto guarded=perk_game(0),unguarded=perk_game(1);guarded.begin(0,m);unguarded.begin(0,m);guarded.battle.hp=40;unguarded.battle.hp=40;guarded.recover(true,m);unguarded.recover(true,m);
    check(guarded.battle.hp-unguarded.battle.hp==4,"Johnny's guard perk should restore health");
    std::cout<<"PASS: "<<checks<<" checks; six-encounter chapter, rewards, training, shop, profiles, defeats and durable saves.\n";
    std::filesystem::remove(path);std::filesystem::remove(path.string()+".bak");return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<" after "<<checks<<" checks\n";return 1;}
}
