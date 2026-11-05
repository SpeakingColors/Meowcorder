cmake_minimum_required(VERSION 3.22)
project(meowcorder VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core test (pure C++, no JUCE)
enable_testing()
add_executable(test_core tests/test_core.cpp)
target_include_directories(test_core PRIVATE src)
add_test(NAME core COMMAND test_core)
add_executable(test_dsp tests/test_dsp.cpp)
target_include_directories(test_dsp PRIVATE src)
add_test(NAME dsp COMMAND test_dsp)

# The plugin and the standalone app. Pass -DMC_JUCE_DIR=/path/to/JUCE to use a local JUCE, otherwise JUCE is fetched.
option(MC_BUILD_JUCE "Build the VST3 plugin and the standalone app" ON)
if(MC_BUILD_JUCE)
  include(FetchContent)
  if(MC_JUCE_DIR)
    add_subdirectory(${MC_JUCE_DIR} juce)
  else()
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.16-security GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
  endif()
  FetchContent_Declare(cat_ui GIT_REPOSITORY https://github.com/SpeakingColors/cat-ui GIT_TAG ba762e1755bbabd6847076e26c5ed7a9c5d6c76d)
  FetchContent_MakeAvailable(cat_ui)

  juce_add_plugin(Meowcorder
    COMPANY_NAME "Studio 87"
    PLUGIN_MANUFACTURER_CODE Spkc
    PLUGIN_CODE McRc
    FORMATS VST3 Standalone
    PRODUCT_NAME "Meowcorder"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Tools
    COPY_PLUGIN_AFTER_BUILD FALSE)
  target_sources(Meowcorder PRIVATE src/plugin/MeowProcessor.cpp src/plugin/MeowEditor.cpp)
  target_include_directories(Meowcorder PRIVATE src)
  target_compile_definitions(Meowcorder PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_USE_FLAC=1 JUCE_USE_OGGVORBIS=1 JUCE_USE_MP3AUDIOFORMAT=1)
  target_link_libraries(Meowcorder PRIVATE cat_ui juce::juce_audio_utils juce::juce_audio_formats
      PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
