#include "infrastructure/persistence/file/file_store.hpp"

#include <fstream>
#include <stdexcept>

namespace fashion_store::infrastructure::persistence::file {

std::vector<std::string> read_all_lines(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return {};
    }

    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open persistence file for reading: " + path.string());
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

void write_all_lines(const std::filesystem::path& path, const std::vector<std::string>& lines) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        throw std::runtime_error("failed to open persistence file for writing: " + path.string());
    }

    for (const auto& line : lines) {
        output << line << '\n';
    }
}

}  // namespace fashion_store::infrastructure::persistence::file
