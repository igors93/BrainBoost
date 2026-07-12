#pragma once

// Cognitive skill categories. Every game trains exactly one category and
// results feed the per-category statistics shown on the dashboard.
enum class GameCategory {
    Memory,
    Reasoning,
    Attention,
    Language,
    Spatial,
    Logic,

    Count
};

inline constexpr int kCategoryCount = static_cast<int>(GameCategory::Count);

// Display name (pt-BR) used across the UI.
inline const char* categoryName(GameCategory category) {
    switch (category) {
        case GameCategory::Memory:    return "Memória";
        case GameCategory::Reasoning: return "Raciocínio";
        case GameCategory::Attention: return "Atenção";
        case GameCategory::Language:  return "Linguagem";
        case GameCategory::Spatial:   return "Espacial";
        case GameCategory::Logic:     return "Lógica";
        default:                      return "?";
    }
}
