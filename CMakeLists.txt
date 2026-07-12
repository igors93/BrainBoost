cmake_minimum_required(VERSION 3.16)
project(BrainBoost VERSION 0.2.0 LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(FREETYPE REQUIRED IMPORTED_TARGET freetype2)

add_executable(brainboost
    src/main.cpp
    src/app/Application.cpp
    src/app/AppContext.cpp
    src/core/GameRegistry.cpp
    src/core/UserProfile.cpp
    src/core/Statistics.cpp
    src/core/Achievements.cpp
    src/core/SaveManager.cpp
    src/games/MentalMathGame.cpp
    src/games/NumberMemoryGame.cpp
    src/games/ReactionTimeGame.cpp
    src/games/SequenceLogicGame.cpp
    src/ui/Renderer.cpp
    src/ui/Input.cpp
    src/ui/Widgets.cpp
    src/ui/Sidebar.cpp
    src/ui/HomeScreen.cpp
    src/ui/GameScreen.cpp
    src/ui/StatsScreen.cpp
    src/ui/AchievementsScreen.cpp
    src/ui/SettingsScreen.cpp
    src/ui/AboutScreen.cpp
)
target_include_directories(brainboost PRIVATE include)
target_link_libraries(brainboost PRIVATE PkgConfig::SDL2 PkgConfig::FREETYPE)
target_compile_options(brainboost PRIVATE -Wall -Wextra)

add_subdirectory(test)
