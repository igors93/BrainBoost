#include <iostream>
#include "app/AppContext.h"
#include "../TestUtils.h"

// Dummy game to avoid full UI dependencies in test
class DummyGame : public Game {
public:
    void frame(float, Renderer&, const Input&, const Rect&) override {}
    bool isFinished() const override { return true; }
    GameResult result() const override { return {100, 5, 5, 20}; }
};

void testActiveGameValidAfterCopy() {
    AppContext context;
    {
        auto games = context.registry.games(); // make copy
        context.startGame(games[0]);
    }
    // The games vector is destroyed here.
    // context.activeGameId should hold the ID, and we can retrieve it safely.
    
    TEST_CHECK(context.activeGame != nullptr);
    TEST_CHECK(!context.activeGameId.empty());
    
    const GameInfo* info = context.registry.findById(context.activeGameId);
    TEST_CHECK(info != nullptr);
    
    // Test save error
    context.saveManager = SaveManager("/invalid_root_dir/test.ini");
    TEST_CHECK(!context.saveProgress());
    TEST_CHECK(!context.lastSaveSucceeded);
    TEST_CHECK(!context.lastSaveError.empty());
    
    std::cout << "testActiveGameValidAfterCopy passed!" << std::endl;
}

int main() {
    std::cout << "Running AppContext tests..." << std::endl;
    testActiveGameValidAfterCopy();
    std::cout << "All AppContext tests passed!" << std::endl;
    return 0;
}
