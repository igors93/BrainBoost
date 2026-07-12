#include "games/MentalMathGame.h"
#include "games/NumberMemoryGame.h"
#include "games/ReactionTimeGame.h"
#include "games/SequenceLogicGame.h"

// Logic tests intentionally do not create a renderer or SDL window. These
// no-op definitions satisfy the virtual table while production builds use the
// real *Render.cpp implementations.
void MentalMathGame::render(Renderer&, const Rect&) const {}
void NumberMemoryGame::render(Renderer&, const Rect&) const {}
void ReactionTimeGame::render(Renderer&, const Rect&) const {}
void SequenceLogicGame::render(Renderer&, const Rect&) const {}
