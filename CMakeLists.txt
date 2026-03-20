cmake_minimum_required(VERSION 3.27)

project(CosmicRays VERSION 1.0.0 LANGUAGES CXX C)

# --- Modular Pamplejuce Configuration ---
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/CPM.cmake)
include(PamplejuceStandard)
include(PamplejuceVersioning)
include(PamplejuceLinux)
include(PamplejuceWindows)

# --- Dependencies ---
CPMAddPackage(
    NAME juce
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG 8.0.6
)

# --- Assets ---
juce_add_binary_data(CosmicRaysData SOURCES assets/Logo.png)

# --- Shared Code (Static Library) ---
add_library(SharedCode STATIC)

set(SHARED_SOURCES
    source/PluginProcessor.cpp
    source/PluginProcessor.h
    source/PluginEditor.cpp
    source/PluginEditor.h
    source/GranularEngine.cpp
    source/GranularEngine.h
    source/helpers/Helpers.h
    source/dsp/BufferReaders.h
    "${VERSION_HEADER}"
)

target_sources(SharedCode PRIVATE ${SHARED_SOURCES})
target_include_directories(SharedCode PUBLIC source)

# Ensure SharedCode has access to BinaryData and Plugin Definitions
target_link_libraries(SharedCode PUBLIC CosmicRaysData)

# Get Version for Definitions
get_project_version(VERSION_STR)

target_compile_definitions(SharedCode PUBLIC
    JucePlugin_Name="Cosmic Rays"
    JucePlugin_VersionString="${VERSION_STR}"
    JucePlugin_Manufacturer="Beechem"
)

target_link_libraries(SharedCode PUBLIC
    juce::juce_audio_utils juce::juce_audio_processors
    juce::juce_gui_extra juce::juce_gui_basics juce::juce_graphics
    juce::juce_events juce::juce_core juce::juce_dsp
    juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
)

if(APPLE)
    target_compile_definitions(SharedCode PUBLIC JUCE_METAL=1)
elseif(UNIX)
    target_include_directories(SharedCode PUBLIC ${GTK3_INCLUDE_DIRS})
    target_link_libraries(SharedCode PUBLIC ${GTK3_LIBRARIES})
    target_compile_definitions(SharedCode PUBLIC JUCE_USE_XSHM=1 JUCE_USE_XRENDER=1)
elseif(WIN32)
    target_compile_definitions(SharedCode PUBLIC JUCE_DIRECT2D=1 JUCE_USE_DIRECTWRITE=1)
endif()

# --- Plugin Target ---
juce_add_plugin(CosmicRays
    COMPANY_NAME "Beechem"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE "BEEH"
    PLUGIN_CODE "COSR"
    FORMATS AU VST3 LV2
    PRODUCT_NAME "Cosmic Rays"
    LV2URI "https://beechem.site/plugins/cosmic-rays"
)

target_link_libraries(CosmicRays PRIVATE SharedCode)
add_dependencies(CosmicRays GenerateVersion)

# --- CPack & Installation ---
set(CPACK_PACKAGE_NAME "CosmicRays")
set(CPACK_PACKAGE_VENDOR "Beechem")
set(CPACK_PACKAGE_VERSION "${VERSION_STR}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

if(APPLE)
    set(CPACK_GENERATOR "productbuild")
    set(CPACK_PACKAGING_INSTALL_PREFIX "") 
    set(PLUGIN_DEST_VST3 "Library/Audio/Plug-Ins/VST3")
    set(PLUGIN_DEST_AU "Library/Audio/Plug-Ins/Components")
    set(PLUGIN_DEST_LV2 "Library/Audio/Plug-Ins/LV2")
elseif(WIN32)
    set(CPACK_GENERATOR "INNOSETUP")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Common Files")
    set(PLUGIN_DEST_VST3 "VST3")
    set(PLUGIN_DEST_LV2 "LV2")
    set(CPACK_INNOSETUP_INSTALL_ROOT "{commoncf}") 
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Beechem")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
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

include(CPack)
