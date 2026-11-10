cmake_minimum_required(VERSION 3.22)

project(CompressorAnalogIRON VERSION 1.0.0 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.11
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(CompressorAnalogIRON
    COMPANY_NAME "VibeCoding"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Vibe
    PLUGIN_CODE Iron
    FORMATS VST3 AU
    PRODUCT_NAME "Compressor Analog IRON"
)

target_sources(CompressorAnalogIRON PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

target_compile_features(CompressorAnalogIRON PRIVATE cxx_std_17)

target_compile_definitions(CompressorAnalogIRON PRIVATE JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1)

target_link_libraries(CompressorAnalogIRON PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
)

juce_generate_juce_header(CompressorAnalogIRON)
