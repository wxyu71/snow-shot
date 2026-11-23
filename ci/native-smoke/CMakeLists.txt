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
    # OCR text shaping needs Qt 6.12 (QTextLayout::RetrieveGlyphIndexes);
    # use the genuine Qt6.4-compatible OCR paragraph-layout implementation.
    "${SNOW_SHOT_ROOT}/snow_shot/src/presentation/ocr/screenshotocrlayout.cpp")
set_target_properties(snow-shot-community-native-clang-only PROPERTIES AUTOMOC OFF)
target_include_directories(snow-shot-community-native-clang-only PRIVATE
    "${SNOW_SHOT_ROOT}/snow_shot/include"
    "${SNOW_SHOT_ROOT}/snow_draw_engine_qt/include")
target_link_libraries(snow-shot-community-native-clang-only PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets)

# Compile-check the real Community PaddleOCR settings widget, not a test copy.
# An object-only target exports compiler flags to compile_commands.json on
# Linux and is explicitly compiled with MSVC in the Windows fast gate.
add_library(snow-shot-community-native-cloud-ocr-widget OBJECT EXCLUDE_FROM_ALL
    "${SNOW_SHOT_ROOT}/snow_shot/src/presentation/components/cloudocrsettingswidget.cpp")
set_target_properties(snow-shot-community-native-cloud-ocr-widget PROPERTIES AUTOMOC OFF)
target_include_directories(snow-shot-community-native-cloud-ocr-widget PRIVATE
    "${SNOW_SHOT_ROOT}/snow_shot/include"
    "${SNOW_SHOT_ROOT}/ant_design_qt/packages/adqt_icon_core/src")
target_link_libraries(snow-shot-community-native-cloud-ocr-widget PRIVATE
    Qt6::Core Qt6::Gui Qt6::Network Qt6::Widgets)

# Compile a real production QObject implementation *and its CMake AUTOMOC output*
# without linking the full GUI. This catches Q_OBJECT/meta-object generation,
# declaration/signature and missing-header errors on both native hosts.
add_library(snow-shot-community-native-canvas-moc OBJECT EXCLUDE_FROM_ALL
    "${SNOW_SHOT_ROOT}/snow_shot/src/presentation/components/canvasstatusreadout.cpp"
    "${SNOW_SHOT_ROOT}/snow_shot/include/snow_shot/presentation/canvasstatusreadout.h")
target_include_directories(snow-shot-community-native-canvas-moc PRIVATE
    "${SNOW_SHOT_ROOT}/snow_shot/include")
target_link_libraries(snow-shot-community-native-canvas-moc PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets)

if(WIN32)
    # Compiling the real Windows variants checks DPAPI and Windows SDK APIs.
    # crypt32 is the actual Windows link dependency of this Settings source.
    target_link_libraries(snow-shot-community-native-settings-tests PRIVATE crypt32)

    add_library(snow-shot-community-native-windows-qos OBJECT EXCLUDE_FROM_ALL
        "${SNOW_SHOT_ROOT}/snow_shot/src/platform/windows/processpowerqos.cpp")
    target_include_directories(snow-shot-community-native-windows-qos PRIVATE
        "${SNOW_SHOT_ROOT}/snow_shot/include")
    target_link_libraries(snow-shot-community-native-windows-qos PRIVATE Qt6::Core)
endif()
