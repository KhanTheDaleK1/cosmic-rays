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

set(PLUGIN_SOURCES
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

# --- Plugin Target ---
juce_add_plugin(CosmicRays
    COMPANY_NAME "Very Good"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE "VGYI"
    PLUGIN_CODE "COSR"
    FORMATS AU VST3 LV2
    PRODUCT_NAME "Cosmic Rays"
    BUNDLE_ID "com.verygood.cosmicrays"
    LV2URI "https://beechem.site/cosmic-rays"
)

target_sources(CosmicRays PRIVATE ${PLUGIN_SOURCES})
target_include_directories(CosmicRays PRIVATE source)

target_link_libraries(CosmicRays PRIVATE
    CosmicRaysData
    juce::juce_audio_utils juce::juce_audio_processors
    juce::juce_gui_extra juce::juce_gui_basics juce::juce_graphics
    juce::juce_events juce::juce_core juce::juce_dsp
    juce::juce_recommended_config_flags
)

if(APPLE)
    target_compile_definitions(CosmicRays PUBLIC JUCE_METAL=1)
elseif(UNIX)
    target_include_directories(CosmicRays PRIVATE ${GTK3_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
    target_link_libraries(CosmicRays PRIVATE ${GTK3_LIBRARIES} ${CURL_LIBRARIES})
    target_compile_definitions(CosmicRays PUBLIC JUCE_USE_XSHM=1 JUCE_USE_XRENDER=1)
elseif(WIN32)
    target_compile_definitions(CosmicRays PUBLIC JUCE_DIRECT2D=1 JUCE_USE_DIRECTWRITE=1)
endif()

add_dependencies(CosmicRays GenerateVersion)

# --- CPack & Installation ---
get_project_version(VERSION_STR)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Very Good")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional Granular Audio Processor")
set(CPACK_PACKAGE_VERSION "${VERSION_STR}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README_Installer.txt")

# --- Component-Based Installation ---
# This allows users to select only the formats they need (AU, VST3, LV2)
set(CPACK_MONOLITHIC_INSTALL OFF)
set(CPACK_COMPONENT_VST3_DISPLAY_NAME "VST3 Plugin")
set(CPACK_COMPONENT_AU_DISPLAY_NAME "Audio Unit (AU) Plugin")
set(CPACK_COMPONENT_LV2_DISPLAY_NAME "LV2 Plugin")

set(CPACK_COMPONENT_VST3_DESCRIPTION "Recommended for Ableton, Cubase, Studio One, and FL Studio.")
set(CPACK_COMPONENT_AU_DESCRIPTION "Required for Logic Pro and GarageBand.")
set(CPACK_COMPONENT_LV2_DESCRIPTION "Used by experimental DAWs and niche macOS workflows.")

set(CPACK_COMPONENT_VST3_GROUP "Plugins")
set(CPACK_COMPONENT_AU_GROUP "Plugins")
set(CPACK_COMPONENT_LV2_GROUP "Plugins")
set(CPACK_COMPONENT_GROUP_PLUGINS_DISPLAY_NAME "Plugin Formats")
set(CPACK_COMPONENT_GROUP_PLUGINS_DESCRIPTION "Select the formats you wish to install on your system.")

if(APPLE)
    set(CMAKE_INSTALL_PREFIX "/")
    set(CPACK_GENERATOR "productbuild")
    set(CPACK_SET_DESTDIR ON)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/") 
    set(PLUGIN_DEST_VST3 "/Library/Audio/Plug-Ins/VST3")
    set(PLUGIN_DEST_AU "/Library/Audio/Plug-Ins/Components")
    set(PLUGIN_DEST_LV2 "/Library/Audio/Plug-Ins/LV2")
    # This enables the "Customize" button in the macOS installer UI
    set(CPACK_PRODUCTBUILD_COMPONENT_INSTALL ON)
elseif(WIN32)
    set(CPACK_GENERATOR "INNOSETUP")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "CosmicRays")
    set(PLUGIN_DEST_VST3 "../../VST3")
    set(PLUGIN_DEST_LV2 "../../LV2")
    set(CPACK_INNOSETUP_INSTALL_ROOT "{commoncf}") 
    set(CPACK_PACKAGE_VERSION_MAJOR "1")
    set(CPACK_PACKAGE_VERSION_MINOR "0")
    set(CPACK_PACKAGE_VERSION_PATCH "0")
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Very Good")
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
