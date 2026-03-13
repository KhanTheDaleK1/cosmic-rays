cmake_minimum_required(VERSION 3.24) # Updated for CPM/Modern JUCE

project(CosmicRays VERSION 0.0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20) # 2026 Standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- CPM.cmake for Modern Dependency Management ---
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME juce
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG 8.0.6
)

# --- Plugin Configuration ---
juce_add_plugin(CosmicRays
    COMPANY_NAME "Beechem"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "BEEH"
    PLUGIN_CODE "COSR"
    FORMATS AU VST3 LV2
    PRODUCT_NAME "Cosmic Rays"
    LV2URI "https://beechen.site/plugins/cosmic-rays"
)

# --- Binary Data ---
juce_add_binary_data(CosmicRaysData
    SOURCES
    Logo.png
)

# --- Sources ---
target_sources(CosmicRays
    PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/GranularEngine.cpp
    Source/GranularEngine.h
)

# --- 2026 Specific Flags ---
target_compile_definitions(CosmicRays
    PUBLIC
    # JUCE 8 Hardware Acceleration
    JUCE_DIRECT2D=1          # Mandatory for 2026 Windows Performance
    JUCE_USE_DIRECTWRITE=1
    
    # Modern Connectivity (Enable WebView if needed later)
    # JUCE_WEB_BROWSER=1
    
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# --- Libraries ---
target_link_libraries(CosmicRays
    PRIVATE
    CosmicRaysData
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_events
    juce::juce_core
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# --- OS Specific Configuration ---
if(APPLE)
    target_compile_definitions(CosmicRays PUBLIC JUCE_METAL=1) # Hardware Acceleration for macOS
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-x11-3.0)
    pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.1)
    
    target_include_directories(CosmicRays PRIVATE ${GTK3_INCLUDE_DIRS} ${WEBKIT_INCLUDE_DIRS})
    target_link_libraries(CosmicRays PRIVATE ${GTK3_LIBRARIES} ${WEBKIT_LIBRARIES})
endif()
