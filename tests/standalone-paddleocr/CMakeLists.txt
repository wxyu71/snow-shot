cmake_minimum_required(VERSION 3.24)
project(snow_shot_community_paddleocr_contract LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

find_package(Qt6 6.4 COMPONENTS Core Gui Network REQUIRED)

get_filename_component(SNOW_SHOT_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
add_executable(paddleocr-api-contract
    "${SNOW_SHOT_REPO_ROOT}/snow_shot/src/network/baidupaddleocrclient.cpp"
    "${SNOW_SHOT_REPO_ROOT}/snow_shot/tests/baidu_paddleocr_client_tests.cpp")
target_include_directories(paddleocr-api-contract PRIVATE
    "${SNOW_SHOT_REPO_ROOT}/snow_shot/include")
target_link_libraries(paddleocr-api-contract PRIVATE
    Qt6::Core Qt6::Gui Qt6::Network)
add_test(NAME paddleocr-api-contract COMMAND paddleocr-api-contract)
set_tests_properties(paddleocr-api-contract PROPERTIES TIMEOUT 75)
