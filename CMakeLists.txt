cmake_minimum_required(VERSION 3.22)

project(BassForge VERSION 1.0.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global build settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------
# JUCE
#
# JUCE is pulled in with FetchContent so the project is self-contained. If you
# already have a local JUCE checkout, set -DBASSFORGE_JUCE_PATH=/path/to/JUCE
# to use it instead of downloading (much faster on repeated configures).
# ---------------------------------------------------------------------------
set(BASSFORGE_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")

if(BASSFORGE_JUCE_PATH)
    message(STATUS "BassForge: using local JUCE at ${BASSFORGE_JUCE_PATH}")
    add_subdirectory(${BASSFORGE_JUCE_PATH} juce_build)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(BassForge
    COMPANY_NAME              "OpenSynthWorks"
    BUNDLE_ID                 com.opensynthworks.bassforge
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    PLUGIN_MANUFACTURER_CODE  Osw0
    PLUGIN_CODE               Bsf1
    FORMATS                   VST3 Standalone
    PRODUCT_NAME              "BassForge")

juce_generate_juce_header(BassForge)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
target_sources(BassForge PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/dsp/BassVoice.cpp)

target_include_directories(BassForge PRIVATE source)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(BassForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# ---------------------------------------------------------------------------
# Link
# ---------------------------------------------------------------------------
target_link_libraries(BassForge PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Offline smoke test (optional): cmake --build build --target bf_smoke_test
# ---------------------------------------------------------------------------
option(BASSFORGE_BUILD_TESTS "Build the offline smoke test" ON)

if(BASSFORGE_BUILD_TESTS)
    juce_add_console_app(bf_smoke_test PRODUCT_NAME "bf_smoke_test")
    juce_generate_juce_header(bf_smoke_test)
    target_sources(bf_smoke_test PRIVATE
        tests/smoke_test.cpp
        source/dsp/BassVoice.cpp)
    target_include_directories(bf_smoke_test PRIVATE source)
    target_compile_definitions(bf_smoke_test PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(bf_smoke_test PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    juce_add_console_app(bf_render_demo PRODUCT_NAME "bf_render_demo")
    juce_generate_juce_header(bf_render_demo)
    target_sources(bf_render_demo PRIVATE
        tools/render_demo.cpp
        source/dsp/BassVoice.cpp)
    target_include_directories(bf_render_demo PRIVATE source)
    target_compile_definitions(bf_render_demo PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(bf_render_demo PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()
