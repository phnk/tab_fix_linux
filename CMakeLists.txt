cmake_minimum_required(VERSION 3.16)
project(tabfix_client LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put the built binary into ./bin (like your Makefile)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Symlink compile_commands.json to project root
add_custom_target(
    link_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
)

# Find pkg-config and GIO (gio-2.0)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GIO REQUIRED gio-2.0)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Source
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(SOURCES ${SRC_DIR}/main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

# Add include dirs and compile flags from pkg-config
target_include_directories(${PROJECT_NAME} PRIVATE ${GIO_INCLUDE_DIRS})
target_compile_options(${PROJECT_NAME} PRIVATE ${GIO_CFLAGS_OTHER})
# Link libraries from pkg-config
target_link_libraries(${PROJECT_NAME} PRIVATE ${GIO_LIBRARIES})

# Qt6
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

# Some convenient compile warnings (similar to -Wall)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall)

# Install target (will install to ${CMAKE_INSTALL_PREFIX}/bin)
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin)

# Optional: a simple "run" target (only when building with CMake)
add_custom_target(run
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running ${PROJECT_NAME}"
)

# Optional: a simple clean target to remove the generated bin folder
add_custom_target(dist-clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_SOURCE_DIR}/bin
    COMMENT "Remove ${CMAKE_SOURCE_DIR}/bin (like make clean)"
)
