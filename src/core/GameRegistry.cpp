#include "core/GameRegistry.h"

#include "games/MentalMathGame.h"
#include "games/NumberMemoryGame.h"
#include "games/ReactionTimeGame.h"
#include "games/SequenceLogicGame.h"

namespace {

template <typename T>
std::function<std::unique_ptr<Game>()> makeFactory() {
    return [] { return std::make_unique<T>(); };
}

}  // namespace

GameRegistry::GameRegistry() {
    games_.push_back({"number_memory", "Memória Numérica",
                      "Lembre-se da sequência de números.",
                      GameCategory::Memory, 0x14532D,
                      makeFactory<NumberMemoryGame>()});

    games_.push_back({"visual_memory", "Memória Visual",
                      "Encontre os pares de figuras iguais.",
                      GameCategory::Memory, 0x3B1D54, nullptr});

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
