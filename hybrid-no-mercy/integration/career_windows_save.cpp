// Native frontend counterpart of the already-used Windows prototype helper.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>

namespace career {
bool replace_save_file(const std::filesystem::path& from,
                       const std::filesystem::path& to) {
    return MoveFileExW(from.c_str(), to.c_str(),
                      MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}
}
#endif
