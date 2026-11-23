cmake_minimum_required(VERSION 3.24)
project(snow_shot_community_native_smoke LANGUAGES CXX)

include(CTest)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Widgets Test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Compile production sources directly. No mocks or duplicated OCR implementations.
set(SNOW_SHOT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
add_executable(snow-shot-community-native-paddleocr-tests
    "${SNOW_SHOT_ROOT}/snow_shot/tests/baidu_paddleocr_client_tests.cpp"
    "${SNOW_SHOT_ROOT}/snow_shot/src/network/baidupaddleocrclient.cpp"
    "${SNOW_SHOT_ROOT}/snow_shot/include/snow_shot/network/baidupaddleocrclient.h")
target_include_directories(snow-shot-community-native-paddleocr-tests PRIVATE
    "${SNOW_SHOT_ROOT}/snow_shot/include")
target_link_libraries(snow-shot-community-native-paddleocr-tests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Network)
add_test(NAME snow-shot-community-native-paddleocr-tests
    COMMAND snow-shot-community-native-paddleocr-tests)
set_tests_properties(snow-shot-community-native-paddleocr-tests PROPERTIES
    TIMEOUT 60 LABELS "native;community;ocr")

add_executable(snow-shot-community-native-settings-tests
    community_native_smoke_tests.cpp
    "${SNOW_SHOT_ROOT}/snow_shot/src/network/communitycloudocrsettings.cpp"
    "${SNOW_SHOT_ROOT}/snow_shot/src/presentation/components/cloudocrenginechoices.cpp"
    "${SNOW_SHOT_ROOT}/snow_shot/include/snow_shot/presentation/components/cloudocrenginechoices.h")
target_include_directories(snow-shot-community-native-settings-tests PRIVATE
    "${SNOW_SHOT_ROOT}/snow_shot/include")
target_link_libraries(snow-shot-community-native-settings-tests PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Test)
add_test(NAME snow-shot-community-native-settings-tests
    COMMAND snow-shot-community-native-settings-tests)
set_tests_properties(snow-shot-community-native-settings-tests PROPERTIES
    TIMEOUT 30 LABELS "native;community;settings"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Runs against production edition/floating-toolbar policy on system Qt, without
# compiling the full static-Qt Windows integration graph or copying schema code.
add_executable(snow-shot-community-native-toolbar-policy-tests
    community_toolbar_policy_tests.cpp)
target_include_directories(snow-shot-community-native-toolbar-policy-tests PRIVATE
    "${SNOW_SHOT_ROOT}/snow_shot/include")
target_compile_definitions(snow-shot-community-native-toolbar-policy-tests PRIVATE
    SNOW_COMMUNITY_SCHEMA_BASELINE_JSON="${SNOW_SHOT_ROOT}/snow_shot/tests/fixtures/configuration-schema-windows-full.json")
target_link_libraries(snow-shot-community-native-toolbar-policy-tests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME snow-shot-community-native-toolbar-policy-tests
    COMMAND snow-shot-community-native-toolbar-policy-tests)
set_tests_properties(snow-shot-community-native-toolbar-policy-tests PROPERTIES
    TIMEOUT 15 LABELS "native;community;schema")

# Production-only translation units with system-Qt headers. CMake exports
# their real Clang compile commands, but EXCLUDE_FROM_ALL avoids compiling or
# linking them in the small native CTest build. The semantic gate still runs
# Clang's parser/type checker on each one before any expensive Windows work.
add_library(snow-shot-community-native-clang-only OBJECT EXCLUDE_FROM_ALL
    "${SNOW_SHOT_ROOT}/snow_shot/src/presentation/core/screenshotgeometry.cpp"
    "${SNOW_SHOT_ROOT}/snow_shot/src/presentation/recording/screenrecordinggeometry.cpp"
    "${SNOW_SHOT_ROOT}/snow_shot/src/presentation/ocr/screenshotocrtexttransform.cpp"
    "${SNOW_SHOT_ROOT}/snow_shot/src/presentation/ocr/screenshotocrtextlayout.cpp"
    "${SNOW_SHOT_ROOT}/snow_shot/src/presentation/components/canvasstatusreadout.cpp")
set_target_properties(snow-shot-community-native-clang-only PROPERTIES AUTOMOC OFF)
target_include_directories(snow-shot-community-native-clang-only PRIVATE
    "${SNOW_SHOT_ROOT}/snow_shot/include"
    "${SNOW_SHOT_ROOT}/snow_draw_engine_qt/include")
target_link_libraries(snow-shot-community-native-clang-only PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets)
