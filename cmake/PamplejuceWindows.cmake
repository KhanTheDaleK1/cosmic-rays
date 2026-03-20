# cmake/PamplejuceWindows.cmake

if(MSVC)
    # --- Static Linking ---
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Function to apply windows specific settings to a target
    function(apply_windows_settings TARGET)
        target_compile_definitions(${TARGET} INTERFACE JUCE_DIRECT2D=1 JUCE_USE_DIRECTWRITE=1)
    endfunction()
endif()
