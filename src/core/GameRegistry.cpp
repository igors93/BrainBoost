#include "core/GameRegistry.h"

#include <random>

#include "games/FillInPuzzleGame.h"
#include "games/MentalMathGame.h"
#include "games/NumberMemoryGame.h"
#include "games/ReactionTimeGame.h"
#include "games/SequenceLogicGame.h"
#include "games/SpatialMemoryGame.h"

namespace {

// Each game is seeded with a fresh random draw per session; startingDifficulty
// carries the player's persisted progress into that session (see
// AppContext::startGame and GameStats::difficultyLevel).
template <typename T>
std::function<std::unique_ptr<Game>(int)> makeFactory() {
    return [](int startingDifficulty) {
        return std::make_unique<T>(std::random_device{}(), startingDifficulty);
    };
}

}  // namespace

GameRegistry::GameRegistry() {
    games_.push_back({"number_memory", "Memória Numérica",
                      "Lembre-se da sequência de números.",
                      GameCategory::Memory, 0x14532D,
                      makeFactory<NumberMemoryGame>()});

    games_.push_back({"visual_memory", "Memória Visual",
                      "Lembre-se e repita a sequência de alertas na grade.",
                      GameCategory::Memory, 0x3B1D54,
                      makeFactory<SpatialMemoryGame>()});

    games_.push_back({"mental_math", "Cálculo Mental",
                      "Resolva operações mentalmente.",
                      GameCategory::Reasoning, 0x16305E,
                      makeFactory<MentalMathGame>()});

    games_.push_back({"crosswords", "Palavras Cruzadas",
                      "Encontre palavras e expanda seu vocabulário.",
                      GameCategory::Language, 0x1A5228, nullptr});

    games_.push_back({"logic_moves", "Movimentos Lógicos",
                      "Descubra o próximo movimento correto.",
                      GameCategory::Logic, 0x54400F, nullptr});

    games_.push_back({"spatial_reasoning", "Raciocínio Espacial",
                      "Encontre o caminho correto até o destino.",
                      GameCategory::Spatial, 0x0E3F4A, nullptr});

    games_.push_back({"logic_sequence", "Sequência Lógica",
                      "Descubra o próximo item da sequência.",
                      GameCategory::Logic, 0x5B1230,
                      makeFactory<SequenceLogicGame>()});

    games_.push_back({"fill_in_puzzle", "Quase Nada",
                      "Preencha a grade com os números certos, usando as pistas e os cruzamentos.",
                      GameCategory::Logic, 0x264653,
                      makeFactory<FillInPuzzleGame>()});

    games_.push_back({"quick_reaction", "Reação Rápida",
                      "Teste seu tempo de reação e reflexos.",
                      GameCategory::Attention, 0x14267A,
                      makeFactory<ReactionTimeGame>()});
}

const GameInfo* GameRegistry::findById(const std::string& id) const {
    for (const GameInfo& info : games_) {
        if (info.id == id) return &info;
    }
    return nullptr;
}
