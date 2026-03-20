# cmake/PamplejuceLinux.cmake

if(UNIX AND NOT APPLE)
    # --- Static Linking ---
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")

    # --- GTK3 & Dependencies ---
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-x11-3.0 webkit2gtk-4.1)
    
    # Function to apply linux specific settings to a target
    function(apply_linux_settings TARGET)
        target_include_directories(${TARGET} INTERFACE ${GTK3_INCLUDE_DIRS})
        target_link_libraries(${TARGET} INTERFACE ${GTK3_LIBRARIES})
        target_compile_definitions(${TARGET} INTERFACE JUCE_USE_XSHM=1 JUCE_USE_XRENDER=1)
    endfunction()
endif()
