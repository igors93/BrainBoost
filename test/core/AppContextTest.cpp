#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../TestUtils.h"
#include "app/AppContext.h"

namespace {
namespace fs = std::filesystem;

fs::path uniqueTestDirectory() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return fs::temp_directory_path() /
           ("brainboost_app_context_" + std::to_string(stamp));
}

std::string readAll(const fs::path& path) {
    std::ifstream file(path);
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

void testActiveGameMetadataSurvivesTemporaryCatalogCopy() {
    AppContext context;
    {
        auto games = context.registry.games();
        context.startGame(games.front());
    }
    TEST_CHECK(context.activeGame != nullptr);
    TEST_CHECK(!context.activeGameId.empty());
    TEST_CHECK(context.registry.findById(context.activeGameId) != nullptr);
}

void testUnsupportedSaveIsNeverOverwritten() {
    const fs::path directory = uniqueTestDirectory();
    fs::create_directories(directory);
    const fs::path savePath = directory / "save.ini";
    const std::string original = "save.version=999\nprofile.xp=9999\n";
    {
        std::ofstream file(savePath);
        file << original;
    }

    AppContext context;
    context.saveManager = SaveManager(savePath.string());
    context.loadProgress();

    TEST_CHECK(context.lastLoadStatus == SaveLoadStatus::UnsupportedVersion);
    TEST_CHECK(context.persistenceReadOnly);
    TEST_CHECK(!context.saveProgress());
    TEST_CHECK(readAll(savePath) == original);

    fs::remove_all(directory);
}

}  // namespace

int main() {
    std::cout << "Running AppContextTest...\n";
    testActiveGameMetadataSurvivesTemporaryCatalogCopy();
    testUnsupportedSaveIsNeverOverwritten();
    std::cout << "All AppContext tests passed!\n";
    return 0;
}
