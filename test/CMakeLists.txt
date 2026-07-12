enable_testing()

add_executable(UserProfileTest core/UserProfileTest.cpp ../src/core/UserProfile.cpp)
target_include_directories(UserProfileTest PRIVATE ../include)
add_test(NAME UserProfileTest COMMAND UserProfileTest)

add_executable(SaveManagerTest core/SaveManagerTest.cpp ../src/core/SaveManager.cpp ../src/core/UserProfile.cpp ../src/core/Statistics.cpp)
target_include_directories(SaveManagerTest PRIVATE ../include)
add_test(NAME SaveManagerTest COMMAND SaveManagerTest)

add_executable(StatisticsTest core/StatisticsTest.cpp ../src/core/Statistics.cpp)
target_include_directories(StatisticsTest PRIVATE ../include)
add_test(NAME StatisticsTest COMMAND StatisticsTest)

add_executable(MathGameTest games/MathGameTest.cpp ../src/games/MentalMathGame.cpp ../src/games/NumberMemoryGame.cpp ../src/games/SequenceLogicGame.cpp ../src/ui/Renderer.cpp ../src/ui/Widgets.cpp ../src/ui/Input.cpp)
target_include_directories(MathGameTest PRIVATE ../include)
target_link_libraries(MathGameTest PRIVATE PkgConfig::SDL2 PkgConfig::FREETYPE)
add_test(NAME MathGameTest COMMAND MathGameTest)

add_executable(UITest ui/UITest.cpp)
target_include_directories(UITest PRIVATE ../include)
target_link_libraries(UITest PRIVATE PkgConfig::SDL2 PkgConfig::FREETYPE)
add_test(NAME UITest COMMAND UITest)

add_executable(AppContextTest core/AppContextTest.cpp ../src/app/AppContext.cpp ../src/core/UserProfile.cpp ../src/core/Statistics.cpp ../src/core/SaveManager.cpp ../src/core/GameRegistry.cpp ../src/games/MentalMathGame.cpp ../src/games/NumberMemoryGame.cpp ../src/games/ReactionTimeGame.cpp ../src/games/SequenceLogicGame.cpp ../src/ui/Renderer.cpp ../src/ui/Widgets.cpp ../src/ui/Input.cpp ../src/core/Achievements.cpp)
target_include_directories(AppContextTest PRIVATE ../include)
target_link_libraries(AppContextTest PRIVATE PkgConfig::SDL2 PkgConfig::FREETYPE)
add_test(NAME AppContextTest COMMAND AppContextTest)

add_executable(ReactionTimeGameTest games/ReactionTimeGameTest.cpp ../src/games/ReactionTimeGame.cpp ../src/ui/Renderer.cpp ../src/ui/Widgets.cpp ../src/ui/Input.cpp)
target_include_directories(ReactionTimeGameTest PRIVATE ../include)
target_link_libraries(ReactionTimeGameTest PRIVATE PkgConfig::SDL2 PkgConfig::FREETYPE)
add_test(NAME ReactionTimeGameTest COMMAND ReactionTimeGameTest)
