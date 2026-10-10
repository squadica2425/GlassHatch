cmake_minimum_required(VERSION 3.22)
project(GlassHatch VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GH_BUILD_PLUGIN "Build the VST3/Standalone plugin (fetches JUCE)" ON)
option(GH_BUILD_TESTS  "Build the JUCE-free DSP unit tests" ON)

if(GH_BUILD_TESTS)
  enable_testing()
  add_executable(gh_dsp_test tests/dsp_test.cpp)
  add_test(NAME dsp COMMAND gh_dsp_test)
endif()

if(GH_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
  FetchContent_MakeAvailable(JUCE)

  juce_add_plugin(GlassHatch
    COMPANY_NAME "YourName"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Ghtc
    PLUGIN_CODE Gh01
    FORMATS VST3 Standalone
    PRODUCT_NAME "GlassHatch")

  target_sources(GlassHatch PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginEditor.cpp)

  target_compile_definitions(GlassHatch PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

  target_link_libraries(GlassHatch
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags)
endif()
