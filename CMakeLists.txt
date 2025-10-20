cmake_minimum_required(VERSION 3.31)
project(Pandora_CPP)

set(CMAKE_CXX_STANDARD 17)

# 添加 Pandora 头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pandora/include)

# 源文件列表（目前只有头文件，模板类全部在头文件实现）
file(GLOB_RECURSE PANDORA_HEADERS pandora/include/pandora/*.h)

# 生成 interface 库（仅头文件，适合模板库）
add_library(pandora INTERFACE)
target_include_directories(pandora INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/pandora/include)

# 可选：如有非模板实现，可添加源文件并生成静态/动态库
# file(GLOB PANDORA_SOURCES pandora/src/*.cpp)
# add_library(pandora_static STATIC ${PANDORA_SOURCES})
# target_include_directories(pandora_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pandora/include)

# 安装规则
install(DIRECTORY pandora/include/pandora DESTINATION include)

# GoogleTest 支持
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# 测试目标
file(GLOB TEST_SOURCES pandora/tests/*.cpp)
add_executable(pandora_tests ${TEST_SOURCES})
target_link_libraries(pandora_tests PRIVATE pandora gtest_main)
add_test(NAME PandoraUnitTests COMMAND pandora_tests)
