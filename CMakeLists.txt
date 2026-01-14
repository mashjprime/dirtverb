cmake_minimum_required(VERSION 3.22)

project(dirtverb VERSION 1.0.0)

# Use C++20 for modern features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../framework ${CMAKE_BINARY_DIR}/JUCE)

# Plugin configuration
juce_add_plugin(dirtverb
    COMPANY_NAME "Dirtverb Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Dtvb
    PLUGIN_CODE Dtvb
    FORMATS VST3
    PRODUCT_NAME "Dirtverb"
)

# Source files
target_sources(dirtverb
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/ShimmerReverb.cpp
        Source/DSP/LofiDegrader.cpp
        Source/DSP/Wavefolder.cpp
        Source/UI/DirtLookAndFeel.cpp
        Source/UI/WaveformVisualizer.cpp
)

# Include directories
target_include_directories(dirtverb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# JUCE modules
target_compile_definitions(dirtverb
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(dirtverb
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
