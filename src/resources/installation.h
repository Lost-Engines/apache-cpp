#pragma once
#include <filesystem>
#include <optional>

namespace apache::resources {
std::filesystem::path default_game_directory();
std::filesystem::path select_game_directory(std::optional<std::filesystem::path> const &requested, std::filesystem::path const &save_directory);
void remember_game_directory(std::filesystem::path const &directory, std::filesystem::path const &save_directory);
void check_reference_installation(std::filesystem::path const &directory);
// Extract only the verified game files. Existing different files are never replaced.
void install_reference_cd(std::filesystem::path const &image, std::filesystem::path const &directory);
} // namespace apache::resources
