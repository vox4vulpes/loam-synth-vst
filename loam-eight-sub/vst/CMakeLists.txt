cmake_minimum_required(VERSION 3.22)
project(LoamEightSub VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG master # ponytail: pin an exact release tag once one is chosen, for reproducible builds
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# The plugin target pulls in JUCE's GUI/windowing modules, which on Linux
# need dev packages (freetype, X11, ALSA, ...) that a plain DSP-only build
# doesn't require. Flip this off to configure just the EngineTest target
# without installing any of that -- CI builds the real plugin with it on.
option(LOAM_BUILD_PLUGIN "Build the VST3/Standalone plugin (needs GUI dev libs on Linux)" ON)

if(LOAM_BUILD_PLUGIN)
    juce_add_plugin(LoamEightSub
        COMPANY_NAME "Loam"
        PRODUCT_NAME "Loam Eight Sub"
        PLUGIN_MANUFACTURER_CODE Loam
        PLUGIN_CODE Le8s
        FORMATS VST3 Standalone
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        VST3_CATEGORIES Instrument Synth
        # Standalone app icon. SVG so JUCE renders every icon size (16/32/48/256 on
        # Windows) directly instead of downscaling one bitmap, which came out blurry.
        ICON_BIG Resources/icon.svg
        ICON_SMALL Resources/icon-small.svg
    )

    target_sources(LoamEightSub PRIVATE
        Source/SynthEngine.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
    )

    target_compile_definitions(LoamEightSub PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(LoamEightSub PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags
    )
endif()

# Lightweight DSP self-test: links only juce_core/juce_audio_basics/juce_dsp,
# so it builds without the GUI/windowing dev packages the full plugin needs.
add_executable(EngineTest
    Tests/EngineTest.cpp
    Source/SynthEngine.cpp
)
target_include_directories(EngineTest PRIVATE Source)
target_compile_definitions(EngineTest PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_link_libraries(EngineTest PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_dsp
)
