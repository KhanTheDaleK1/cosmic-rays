# cmake/PamplejuceVersioning.cmake

find_package(Python3 REQUIRED)

set(VERSION_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/source/Version.h")

add_custom_target(GenerateVersion
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_version.py" "${VERSION_HEADER}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Updating version header"
    VERBATIM
)

# Function to extract version from version.txt
function(get_project_version OUT_VAR)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" VERSION_STR)
    string(STRIP "${VERSION_STR}" VERSION_STR)
    set(${OUT_VAR} "${VERSION_STR}" PARENT_SCOPE)
endfunction()
