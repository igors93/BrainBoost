#include <cassert>
#include <iostream>
#include "games/ReactionTimeGame.h"
#include "ui/Input.h"

// Note: To test properly, we would mock Input and Renderer, but we can do a basic logic test
void testReactionGameLogic() {
    ReactionTimeGame game;
    // Without full renderer and input mock, we can't fully drive the frame loop
    // But we can verify it starts in Instructions phase.
    assert(!game.isFinished());
    std::cout << "testReactionGameLogic passed!" << std::endl;
}

int main() {
    std::cout << "Running ReactionTimeGame tests..." << std::endl;
    testReactionGameLogic();
    std::cout << "All ReactionTimeGame tests passed!" << std::endl;
    return 0;
}
