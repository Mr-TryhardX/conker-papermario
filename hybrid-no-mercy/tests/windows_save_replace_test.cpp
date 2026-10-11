#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace career {
bool replace_save_file(const std::filesystem::path&, const std::filesystem::path&);
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const std::filesystem::path folder(argv[1]);
    // Run only in a new disposable folder; never inspect or replace user saves.
    if (std::filesystem::exists(folder)) return 3;
    std::filesystem::create_directories(folder);
    const auto target = folder / "save.json", source = folder / "save.tmp";
    { std::ofstream(target) << "old"; std::ofstream(source) << "new"; }
    if (!career::replace_save_file(source, target) || std::filesystem::exists(source)) return 4;
    std::string value;
    { std::ifstream(target) >> value; }
    if (value != "new") return 5;
    if (career::replace_save_file(source, target)) return 6;
    { std::ifstream(target) >> value; }
    if (value != "new") return 7;
    std::filesystem::remove_all(folder);
    std::cout << "PASS: Windows save replacement updates an existing file; failed replacement preserves it.\n";
}
