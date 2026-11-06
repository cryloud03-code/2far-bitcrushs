cmake_minimum_required(VERSION 3.22)

# Runtime estático en Windows: el .vst3 no necesita instalar Visual C++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(TwoFarBitcrusher VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# Las tres imágenes se incrustan dentro del plugin (no hay que distribuir archivos aparte).
juce_add_binary_data(TwoFarAssets SOURCES
    Assets/bus.jpg
    Assets/anime.jpg
    Assets/flag.jpg)

juce_add_plugin(TwoFarBitcrusher
    COMPANY_NAME "2far"
    PRODUCT_NAME "2far Bitcrusher"
    PLUGIN_MANUFACTURER_CODE Twfr
    PLUGIN_CODE Tfbc
    FORMATS VST3
    VST3_CATEGORIES Fx Distortion
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(TwoFarBitcrusher)

target_sources(TwoFarBitcrusher PRIVATE
    Source/BitCrusherDSP.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

target_compile_definitions(TwoFarBitcrusher PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(MSVC)
    target_compile_options(TwoFarBitcrusher PRIVATE /utf-8)
endif()

target_link_libraries(TwoFarBitcrusher
    PRIVATE
        TwoFarAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
