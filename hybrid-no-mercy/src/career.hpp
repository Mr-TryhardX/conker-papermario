#pragma once
#include <array>
#include <filesystem>
#include <string>
#include <vector>
#include <json.hpp>

namespace career {
constexpr int stat_count = 6;
constexpr int fighter_count = 6;
constexpr int costume_count = 4;
constexpr int move_count = 8;
constexpr int encounter_count = 6;
enum Stat { Power, Grapple, Technique, Defense, Speed, Endurance };
struct FighterDef { const char* name; const char* style; int price; std::array<int,stat_count> base; const char* perk; const char* perk_detail; };
struct MoveDef { const char* name; int stat; int power; int stamina; int price; int required_level; };
struct EncounterDef { const char* name; int health; int attack; int defense; int xp; int coins; };
extern const std::array<const char*,stat_count> stat_names;
extern const std::array<FighterDef,fighter_count> fighters;
extern const std::array<MoveDef,move_count> moves;
extern const std::array<EncounterDef,encounter_count> encounters;
struct Fighter {
    bool unlocked = false;
    int level = 1, xp = 0, points = 0, costume = 0;
    std::array<int,stat_count> training{};
    std::array<bool,move_count> learned{true,true,true,false,false,false,false,false};
};
struct State {
    int version = 2, selected = 0, coins = 100, medkits = 2;
    int wins = 0, losses = 0;
    std::array<bool,encounter_count> cleared{};
    std::array<Fighter,fighter_count> roster{};
    State();
};
struct Battle {
    bool active = false;
    bool perk_used = false;
    int encounter = 0, hp = 0, stamina = 0, enemy_hp = 0, turn = 0;
    std::vector<std::string> log;
};
class Game {
public:
    State state;
    Battle battle;
    int stat(int index) const;
    int max_hp() const;
    int max_stamina() const;
    int next_xp() const;
    bool select(int fighter, std::string& message);
    bool select_costume(int costume, std::string& message);
    bool upgrade(int index, std::string& message);
    bool buy_move(int index, std::string& message);
    bool buy_medkit(std::string& message);
    bool begin(int encounter, std::string& message);
    bool attack(int move, std::string& message);
    bool recover(bool guard, std::string& message);
    bool heal(std::string& message);
    void retreat(std::string& message);
    nlohmann::json serialize() const;
    bool restore(const nlohmann::json& data, std::string& error);
    bool save(const std::filesystem::path& path, std::string& error) const;
    bool load(const std::filesystem::path& path, std::string& error);
private:
    void enemy_turn(bool guard);
    void victory();
};
}
