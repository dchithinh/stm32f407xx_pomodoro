# Define pomodoro paths independently 
set(POMODORO_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Root directory for pomodoro library")

# Verify pomodoro directory exists
if(NOT EXISTS ${POMODORO_ROOT_DIR})
    message(FATAL_ERROR "POMODORO_ROOT_DIR '${POMODORO_ROOT_DIR}' does not exist. Please set correct path.")
endif()

# Component directories relative to POMODORO_ROOT_DIR
set(POMODORO_COMPONENTS
    Core
    HAL
    UI
    assets
)

# Add pomodoro library
add_library(pomodoro_app STATIC)

# Collect sources from each component
foreach(component ${POMODORO_COMPONENTS})
    file(GLOB_RECURSE COMPONENT_SOURCES "${POMODORO_ROOT_DIR}/${component}/*.c")
    target_sources(pomodoro_app PRIVATE ${COMPONENT_SOURCES})
    target_include_directories(pomodoro_app 
        PUBLIC 
            "${POMODORO_ROOT_DIR}/${component}"
            "${POMODORO_ROOT_DIR}/${component}/Inc"  # Add Inc subdirectory if you use it
    )
endforeach()

# Add LVGL dependency and include paths
target_link_libraries(pomodoro_app PUBLIC lvgl)
target_include_directories(pomodoro_app PUBLIC 
    ${CMAKE_SOURCE_DIR}     # Root project directory for lvgl.h
    ${CMAKE_SOURCE_DIR}/lvgl  # LVGL directory
    ${POMODORO_ROOT_DIR}  # Add root pomodoro directory for direct includes
)

# Export pomodoro_app target
set_target_properties(pomodoro_app PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)