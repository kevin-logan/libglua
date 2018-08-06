#include "glua/FileUtil.h"

#include <fstream>
#include <streambuf>

namespace kdk::file_util
{
auto read_all(std::string_view filename) -> std::string
{
    std::ifstream file{filename.data()};

    return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

auto write_all(std::string_view filename, std::string_view data) -> void
{
    std::ofstream file{filename.data()};

    std::copy(data.begin(), data.end(), std::ostreambuf_iterator<char>{file});
}

auto append_all(std::string_view filename, std::string_view data) -> void
{
    std::ofstream file{filename.data(), std::ios_base::app};

    std::copy(data.begin(), data.end(), std::ostreambuf_iterator<char>{file});
}

} // namespace kdk::file_util
