cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)

project(strategy)

# Clang deez nuts
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)

set(GLFW_ROOT_DIR external/glfw)
set(GLM_ROOT_DIR external/glm)
set(IMGUI_ROOT_DIR external/imgui)
set(VKBOOTSTRAP_ROOT_DIR external/vk-bootstrap)

file(GLOB_RECURSE HEADER_FILES 
	${CMAKE_SOURCE_DIR}/include/*.h
	${CMAKE_SOURCE_DIR}/include/*.hpp
)


IF (WIN32)
    set(LIBS glfw vulkan-1 glm)
    set(Vulkan_INCLUDE_DIRS C:/VulkanSDK/1.3.268.0/Include)
    set(Vulkan_LIB_DIRS C:/VulkanSDK/1.3.268.0/Lib)

    file(GLOB_RECURSE SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/platform/windows.cpp

        ${IMGUI_ROOT_DIR}/imgui.cpp
        ${IMGUI_ROOT_DIR}/imgui_demo.cpp
        ${IMGUI_ROOT_DIR}/imgui_draw.cpp
        ${IMGUI_ROOT_DIR}/imgui_tables.cpp
        ${IMGUI_ROOT_DIR}/imgui_widgets.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_glfw.cpp

        ${VKBOOTSTRAP_ROOT_DIR}/src/VkBootstrap.cpp
    )
ELSE()
    set(LIBS glfw X11 vulkan glm)

    file(GLOB_RECURSE SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/platform/linux.cpp

        ${IMGUI_ROOT_DIR}/imgui.cpp
        ${IMGUI_ROOT_DIR}/imgui_demo.cpp
        ${IMGUI_ROOT_DIR}/imgui_draw.cpp
        ${IMGUI_ROOT_DIR}/imgui_tables.cpp
        ${IMGUI_ROOT_DIR}/imgui_widgets.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_ROOT_DIR}/backends/imgui_impl_glfw.cpp

        ${VKBOOTSTRAP_ROOT_DIR}/src/VkBootstrap.cpp
    )
ENDIF()

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})


# Add the External Libraries / Files Directory
add_subdirectory(${GLFW_ROOT_DIR})
add_subdirectory(${GLM_ROOT_DIR})
add_subdirectory(${VKBOOTSTRAP_ROOT_DIR})

target_include_directories(${PROJECT_NAME} 
	PUBLIC ${GLFW_ROOT_DIR}/include
    PUBLIC ${IMGUI_ROOT_DIR}
    PUBLIC ${IMGUI_ROOT_DIR}/backends
    PUBLIC ${GLM_ROOT_DIR}/include
    PUBLIC ${VKBOOTSTRAP_ROOT_DIR}/src
    PUBLIC .
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /W4)
else()
    target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Wpedantic)
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE ${Vulkan_INCLUDE_DIRS}
)

target_link_directories(${PROJECT_NAME} 
	PRIVATE ${GLFW_ROOT_DIR}/src
    PRIVATE ${GLM_ROOT_DIR}/src
    PRIVATE ${Vulkan_LIB_DIRS}
)

target_link_libraries(${PROJECT_NAME} ${LIBS})

