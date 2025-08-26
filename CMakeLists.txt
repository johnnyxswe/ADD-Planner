cmake_minimum_required(VERSION 4.0)
project(ADD_Todo_App)

set(CMAKE_CXX_STANDARD 20)

#add_compile_definitions(NDEBUG)

find_package(Vulkan REQUIRED)

include(cmake/Shaders.cmake)
include_directories(imgui)

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/libs CACHE PATH "Missing description." FORCE)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY "https://github.com/glfw/glfw.git"
        GIT_TAG "3.4"
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
        GIT_TAG "v1.12.0"
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
        glm
        GIT_REPOSITORY "https://github.com/g-truc/glm.git"
        GIT_TAG "1.0.1"
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)


add_executable(${PROJECT_NAME}
        imgui/imconfig.h
        imgui/imgui.cpp
        imgui/imgui.h
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_internal.h
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/imstb_rectpack.h
        imgui/imstb_textedit.h
        imgui/imstb_truetype.h
        main.cpp
        imgui/imgui_impl_glfw.cpp
        imgui/imgui_impl_glfw.h
        imgui/imgui_impl_vulkan.cpp
        imgui/imgui_impl_vulkan.h
        src/application.cpp
        src/application.h
        src/graphics.cpp
        src/graphics.h
        src/imgui_renderer.cpp
        src/imgui_renderer.h
        src/sqlite3.c
        src/card_database.cpp
        src/card_database.h
        src/todo_card.h)

file(GLOB_RECURSE ShaderSources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
)

add_shaders(VulkanEngineShaders ${ShaderSources})
add_dependencies(${PROJECT_NAME} VulkanEngineShaders)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)

file(COPY "resources/fonts/Inter/Inter-VariableFont_opsz,wght.ttf" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
