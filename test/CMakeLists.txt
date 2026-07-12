enable_testing()

add_executable(UserProfileTest core/UserProfileTest.cpp ../src/core/UserProfile.cpp)
target_include_directories(UserProfileTest PRIVATE ../include)
add_test(NAME UserProfileTest COMMAND UserProfileTest)

add_executable(SaveManagerTest core/SaveManagerTest.cpp ../src/core/SaveManager.cpp ../src/core/UserProfile.cpp ../src/core/Statistics.cpp)
target_include_directories(SaveManagerTest PRIVATE ../include)
add_test(NAME SaveManagerTest COMMAND SaveManagerTest)

add_executable(MathGameTest games/MathGameTest.cpp)
target_include_directories(MathGameTest PRIVATE ../include)
add_test(NAME MathGameTest COMMAND MathGameTest)

add_executable(UITest ui/UITest.cpp)
target_include_directories(UITest PRIVATE ../include)
add_test(NAME UITest COMMAND UITest)
