cmake_minimum_required(VERSION 3.22)
project(Meowcorder VERSION 0.5.0 LANGUAGES C CXX)

# Meowcorder: audio recorder and editor plugin (VST3 + standalone). The engine in src/core is pure C++ with no JUCE.
include(FetchContent)
FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.16-security GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)

juce_add_binary_data(MeowcorderAssets SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/BarlowCondensed-Medium.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/BarlowCondensed-SemiBold.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/BarlowCondensed-Bold.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/JetBrainsMono-Regular.ttf)
set_target_properties(MeowcorderAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(MEOW_SOURCES
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp)

juce_add_plugin(Meowcorder
    COMPANY_NAME "Studio 87"
    PLUGIN_MANUFACTURER_CODE Spco
    PLUGIN_CODE Mwcd
    FORMATS VST3 Standalone
    PRODUCT_NAME "Meowcorder"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Tools
    BUNDLE_ID com.studio87.meowcorder)
target_sources(Meowcorder PRIVATE ${MEOW_SOURCES})
target_include_directories(Meowcorder PRIVATE src)
target_compile_features(Meowcorder PUBLIC cxx_std_17)
target_compile_definitions(Meowcorder PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
target_link_libraries(Meowcorder PRIVATE MeowcorderAssets
    juce::juce_audio_utils juce::juce_audio_formats juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Headless checks of the plugin glue (record, edit, play) without opening a window or a host.
juce_add_console_app(MeowcorderPluginTest PRODUCT_NAME "meowcorder_plugin_test")
target_sources(MeowcorderPluginTest PRIVATE tests/plugin_test.cpp src/plugin/PluginProcessor.cpp)
target_include_directories(MeowcorderPluginTest PRIVATE src)
target_compile_features(MeowcorderPluginTest PUBLIC cxx_std_17)
target_compile_definitions(MeowcorderPluginTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 MEOW_NO_EDITOR=1)
target_link_libraries(MeowcorderPluginTest PRIVATE juce::juce_audio_utils juce::juce_audio_formats
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# Renders the real window to a PNG, to compare it with the mockup (tools/snapshot.cpp).
juce_add_console_app(MeowcorderSnapshot PRODUCT_NAME "meowcorder_snapshot")
target_sources(MeowcorderSnapshot PRIVATE tools/snapshot.cpp ${MEOW_SOURCES})
target_include_directories(MeowcorderSnapshot PRIVATE src)
target_compile_features(MeowcorderSnapshot PUBLIC cxx_std_17)
target_compile_definitions(MeowcorderSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1 JucePlugin_VersionString="${PROJECT_VERSION}")
target_link_libraries(MeowcorderSnapshot PRIVATE MeowcorderAssets juce::juce_audio_utils juce::juce_audio_formats juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
