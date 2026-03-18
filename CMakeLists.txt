cmake_minimum_required(VERSION 3.27) # Updated for CPack InnoSetup Generator & CPM/Modern JUCE

project(CosmicRays VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20) # 2026 Standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Static Linking ---
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(UNIX AND NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# --- CPM.cmake for Modern Dependency Management ---
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME juce
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG 8.0.6
)

# --- Version Generation ---
find_package(Python3 REQUIRED)
set(VERSION_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Source/Version.h")

# Use a custom target directly to ensure it runs every time
add_custom_target(GenerateVersion
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/generate_version.py" "${VERSION_HEADER}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Updating version header: ${VERSION_HEADER}"
    VERBATIM
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
    LV2URI "https://beechem.site/plugins/cosmic-rays"
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
    "${VERSION_HEADER}"
)

add_dependencies(CosmicRays GenerateVersion)

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

# --- CPack Configuration ---
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" VERSION_STR)
string(STRIP "${VERSION_STR}" VERSION_STR)

set(CPACK_PACKAGE_NAME "CosmicRays")
set(CPACK_PACKAGE_VENDOR "Beechem")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cosmic Rays Audio Plugin")
set(CPACK_PACKAGE_VERSION "${VERSION_STR}")

# Modern Platform-Specific Plugin Path Logic
if(APPLE)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "CosmicRays")
    set(PLUGIN_DEST_VST3 "Library/Audio/Plug-Ins/VST3")
    set(PLUGIN_DEST_AU "Library/Audio/Plug-Ins/Components")
    set(PLUGIN_DEST_LV2 "Library/Audio/Plug-Ins/LV2")
elseif(WIN32)
    set(PLUGIN_DEST_VST3 "Common Files/VST3")
    set(PLUGIN_DEST_LV2 "Common Files/LV2")
else() # Linux
    set(PLUGIN_DEST_VST3 "lib/vst3")
    set(PLUGIN_DEST_LV2 "lib/lv2")
endif()

if(TARGET CosmicRays_VST3)
    install(TARGETS CosmicRays_VST3 DESTINATION "${PLUGIN_DEST_VST3}" COMPONENT VST3)
endif()

if(TARGET CosmicRays_AU)
    install(TARGETS CosmicRays_AU DESTINATION "${PLUGIN_DEST_AU}" COMPONENT AU)
endif()

if(TARGET CosmicRays_LV2)
    install(TARGETS CosmicRays_LV2 DESTINATION "${PLUGIN_DEST_LV2}" COMPONENT LV2)
endif()

if(APPLE)
    set(CPACK_GENERATOR "productbuild")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/")
elseif(WIN32)
    # INNOSETUP requires CMake 3.27+
    set(CPACK_GENERATOR "INNOSETUP")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Beechem")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
