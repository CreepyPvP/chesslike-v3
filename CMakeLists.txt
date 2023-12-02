cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(strategy)

# Clang deez nuts
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include debug information
set(CMAKE_BUILD_TYPE Debug)

# Variables for Paths of External Libraries
set(GLFW_ROOT_DIR external/glfw)
set(IMGUI_ROOT_DIR external/imgui)

# Add all Header Files
file(GLOB_RECURSE HEADER_FILES 
	${CMAKE_SOURCE_DIR}/include/*.h
	${CMAKE_SOURCE_DIR}/include/*.hpp
)


IF (WIN32)
    # Windows libraries:
    find_package(OpenGL REQUIRED)
    set(LIBS glfw vulkan)

    file(GLOB_RECURSE SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/platform/windows.cpp

        # IMGUI
        ${IMGUI_ROOT_DIR}/imgui.cpp
        ${IMGUI_ROOT_DIR}/imgui_demo.cpp
        ${IMGUI_ROOT_DIR}/imgui_draw.cpp
        ${IMGUI_ROOT_DIR}/imgui_tables.cpp
        ${IMGUI_ROOT_DIR}/imgui_widgets.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_glfw.cpp
    )
ELSE()
    # Linux libraries:
    set(LIBS glfw X11 vulkan)

    file(GLOB_RECURSE SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/platform/linux.cpp

        # IMGUI
        ${IMGUI_ROOT_DIR}/imgui.cpp
        ${IMGUI_ROOT_DIR}/imgui_demo.cpp
        ${IMGUI_ROOT_DIR}/imgui_draw.cpp
        ${IMGUI_ROOT_DIR}/imgui_tables.cpp
        ${IMGUI_ROOT_DIR}/imgui_widgets.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_glfw.cpp
    )
ENDIF()

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})



# Add the External Libraries / Files Directory
add_subdirectory(${GLFW_ROOT_DIR})

# Add the Include Directories for the Libraries / Files
#Used for adding #include into the cpp files

#		Include Files From Within the Workspace Folder

target_include_directories(${PROJECT_NAME} 
	PUBLIC ${GLFW_ROOT_DIR}/include
    PUBLIC ${IMGUI_ROOT_DIR}
    PUBLIC ${IMGUI_ROOT_DIR}/backends
    PUBLIC include
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Wpedantic)
endif()

#		Include Files from Outside the Workspace Folder -> Like Vulkan SDK

#target_include_directories(${PROJECT_NAME}
#	PRIVATE ${Vulkan_INCLUDE_DIRS}
#)

# Add the Link Directories for the Libraries / Files
#Used for creation of Static/Dynamic Libraries

#		Include Files From Within the Workspace Folder

target_link_directories(${PROJECT_NAME} 
	PRIVATE ${GLFW_ROOT_DIR}/src
)

target_link_libraries(${PROJECT_NAME} ${LIBS})

