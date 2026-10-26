cmake_minimum_required(VERSION 3.22)

project(Cerberus VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The local Linux dependency sysroot is only a build-time convenience. Do not
# bake its absolute paths into distributed binaries; resolve runtime libraries
# through the system loader instead.
set(CMAKE_SKIP_RPATH TRUE)

option(CERBERUS_BUILD_TESTS "Build the offline DSP test executable" ON)
option(CERBERUS_BUILD_SCREENSHOT "Build the headless editor renderer" ON)
option(CERBERUS_BUILD_HOST_TEST "Build the VST3 host smoke test" ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.8
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(CerberusAssets
    SOURCES
        Resources/LatoRegular.ttf
        Resources/LatoSemibold.ttf
        Resources/LatoBlack.ttf
        Resources/Lato-OFL.txt)

set(CERBERUS_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Parameters.h
    Source/Presets.h
    Source/dsp/AutoWah.h
    Source/dsp/Compressor.h
    Source/dsp/Distortion.h
    Source/ui/Theme.cpp
    Source/ui/Theme.h
    Source/ui/StudioLookAndFeel.cpp
    Source/ui/StudioLookAndFeel.h
    Source/ui/Controls.cpp
    Source/ui/Controls.h
    Source/ui/EffectPanel.cpp
    Source/ui/EffectPanel.h
    Source/ui/WaveformDisplay.cpp
    Source/ui/WaveformDisplay.h)

set(CERBERUS_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

juce_add_plugin(Cerberus
    COMPANY_NAME "BLS Audio"
    BUNDLE_ID "com.blsaudio.cerberus"
    PLUGIN_MANUFACTURER_CODE Blsa
    PLUGIN_CODE Crbs
    FORMATS VST3 Standalone
    PRODUCT_NAME "Cerberus"
    DESCRIPTION "Three-stage auto-wah, compressor and distortion"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Fx")

target_sources(Cerberus PRIVATE ${CERBERUS_SOURCES})
target_include_directories(Cerberus PRIVATE Source)

target_compile_definitions(Cerberus PUBLIC ${CERBERUS_DEFINITIONS})

target_link_libraries(Cerberus
    PRIVATE
        CerberusAssets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if(CERBERUS_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(CerberusTests PRODUCT_NAME "CerberusTests")
    target_sources(CerberusTests PRIVATE Tests/DspTests.cpp)
    target_include_directories(CerberusTests PRIVATE Source)
    target_compile_definitions(CerberusTests PRIVATE ${CERBERUS_DEFINITIONS})
    target_link_libraries(CerberusTests
        PRIVATE
            juce::juce_audio_basics
            juce::juce_dsp
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_test(NAME cerberus_dsp COMMAND $<TARGET_FILE:CerberusTests>)
endif()

if(CERBERUS_BUILD_HOST_TEST)
    juce_add_console_app(CerberusHostSmokeTest PRODUCT_NAME "CerberusHostSmokeTest")

    target_sources(CerberusHostSmokeTest PRIVATE Tools/HostSmokeTest.cpp)
    target_compile_definitions(CerberusHostSmokeTest PRIVATE ${CERBERUS_DEFINITIONS} JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(CerberusHostSmokeTest
        PRIVATE
            juce::juce_audio_utils
            juce::juce_gui_extra
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_dependencies(CerberusHostSmokeTest Cerberus_VST3)
endif()

if(CERBERUS_BUILD_SCREENSHOT)
    juce_add_gui_app(CerberusScreenshot
        PRODUCT_NAME "Cerberus Screenshot"
        COMPANY_NAME "BLS Audio")

    target_sources(CerberusScreenshot PRIVATE ${CERBERUS_SOURCES} Tools/RenderEditor.cpp)
    target_include_directories(CerberusScreenshot PRIVATE Source)

    target_compile_definitions(CerberusScreenshot PRIVATE ${CERBERUS_DEFINITIONS})

    target_link_libraries(CerberusScreenshot
        PRIVATE
            CerberusAssets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()
