cmake_minimum_required(VERSION 3.22)

project(Cinder VERSION 1.0.0)

# Use C++20 for modern features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../framework ${CMAKE_BINARY_DIR}/JUCE)

# Embed fonts as binary data
juce_add_binary_data(CinderFonts
    SOURCES
        Resources/Fonts/DMSans-Light.ttf
        Resources/Fonts/DMSans-Regular.ttf
        Resources/Fonts/DMSans-Medium.ttf
        Resources/Fonts/SpaceMono-Regular.ttf
        Resources/Fonts/SpaceMono-Bold.ttf
)

# Plugin configuration
juce_add_plugin(Cinder
    COMPANY_NAME "Substrate Audio"
    BUNDLE_ID "com.substrateaudio.cinder"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE SbAu
    PLUGIN_CODE Cind
    FORMATS VST3
    PRODUCT_NAME "Cinder"
)

# Source files
target_sources(Cinder
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Include directories
target_include_directories(Cinder
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/UI
)

# JUCE modules
target_compile_definitions(Cinder
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Cinder
    PRIVATE
        CinderFonts
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
