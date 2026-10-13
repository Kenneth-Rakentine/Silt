cmake_minimum_required(VERSION 3.22)
project(Silt VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static MSVC runtime so Silt.exe runs on any Windows machine without the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE: use a local checkout if given (-DSILT_JUCE_DIR=...), otherwise fetch the pinned release.
set(SILT_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout")
if(SILT_JUCE_DIR)
    add_subdirectory(${SILT_JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(Silt
    COMPANY_NAME "Kenneth Rakentine"
    PRODUCT_NAME "Silt"
    BUNDLE_ID "com.kennethrakentine.silt"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Krak
    PLUGIN_CODE Silt
    FORMATS Standalone VST3)

set(SILT_ENGINE_SOURCES
    Source/Engine/SiltEngine.cpp Source/Engine/Movement.cpp Source/Engine/Feedback.cpp Source/Engine/Memory.cpp
    Source/Catch/CatchSection.cpp Source/Catch/nebulae_engine.cpp Source/Catch/nebulae_dsp.cpp)

target_sources(Silt PRIVATE
    ${SILT_ENGINE_SOURCES}
    Source/App/PluginProcessor.cpp
    Source/App/PluginEditor.cpp
    Source/App/Widgets.cpp)

target_compile_definitions(Silt PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    _USE_MATH_DEFINES)

if(MSVC)
    target_compile_options(Silt PRIVATE /utf-8)
endif()

target_link_libraries(Silt
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

# Offline check of the C++ engine against the browser lab (see tools/).
add_executable(silt_engine_check tools/engine_check.cpp ${SILT_ENGINE_SOURCES})
add_executable(silt_feature_check tools/feature_check.cpp ${SILT_ENGINE_SOURCES})
set_target_properties(silt_feature_check PROPERTIES EXCLUDE_FROM_ALL TRUE)
foreach(t silt_engine_check silt_feature_check)
    target_compile_definitions(${t} PRIVATE _USE_MATH_DEFINES)
endforeach()
set_target_properties(silt_engine_check PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Renders the factory patches through the whole app processor (see tools/).
juce_add_console_app(silt_app_check PRODUCT_NAME "silt_app_check")
target_sources(silt_app_check PRIVATE tools/app_check.cpp ${SILT_ENGINE_SOURCES} Source/App/PluginProcessor.cpp Source/App/PluginEditor.cpp Source/App/Widgets.cpp)
target_compile_definitions(silt_app_check PRIVATE _USE_MATH_DEFINES JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Silt" JucePlugin_IsSynth=1)
target_link_libraries(silt_app_check PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
set_target_properties(silt_app_check PROPERTIES EXCLUDE_FROM_ALL TRUE)
if(MSVC)
    target_compile_options(silt_app_check PRIVATE /utf-8)
endif()
