#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fashion_store::infrastructure::persistence::file {

std::vector<std::string> read_all_lines(const std::filesystem::path& path);

void write_all_lines(const std::filesystem::path& path, const std::vector<std::string>& lines);

}  // namespace fashion_store::infrastructure::persistence::file
