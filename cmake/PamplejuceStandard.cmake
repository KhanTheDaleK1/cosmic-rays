# cmake/PamplejuceStandard.cmake

# --- C++ Standard ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Common Compile Flags ---
if(MSVC)
    add_compile_options(/W4) # High warning level (removed /WX error)
else()
    add_compile_options(-Wall -Wextra -Wpedantic) # (removed -Werror)
endif()

# --- JUCE Specific Global Flags ---
set(JUCE_VST3_CAN_REPLACE_VST2 0 CACHE INTERNAL "")
set(JUCE_WEB_BROWSER 0 CACHE INTERNAL "")
set(JUCE_USE_CURL 0 CACHE INTERNAL "")
