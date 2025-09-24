cmake_minimum_required(VERSION 3.20)  # Fixed: 4.0 doesn't exist
project(ADD_Todo_App)

set(CMAKE_CXX_STANDARD 20)

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

FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG "2.6.x"
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SFML)

set(SOURCES
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
        src/todo_card.h
        src/pomodoro_timer.h
        src/utilities.cpp
        src/utilities.h
        src/project.h
        src/audio_engine.cpp
        src/audio_engine.h)

# Determine if we should create a bundle
# Only create bundle for Release builds on macOS
set(CREATE_BUNDLE FALSE)
if(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CREATE_BUNDLE TRUE)
endif()

# Create the executable
if(CREATE_BUNDLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES})
    message(STATUS "Creating macOS bundle for Release build")
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
    message(STATUS "Creating regular executable for ${CMAKE_BUILD_TYPE} build")
endif()


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
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-audio sfml-window sfml-system)

# Add these lines after your existing target_link_libraries calls
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreFoundation")
    # Add framework dependencies that SFML needs on macOS
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework AudioUnit")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreAudio")
endif()


# Copy OpenAL DLL on Windows
if(WIN32)
    add_custom_command(
            TARGET ${PROJECT_NAME}
            COMMENT "Copy OpenAL DLL"
            PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            VERBATIM)
endif()


file(COPY "resources/fonts/Inter/Inter-VariableFont_opsz,wght.ttf" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "sounds/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/sounds")

# macOS bundle configuration - only for Release builds
if(CREATE_BUNDLE)
    # Set bundle properties
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.in
            MACOSX_BUNDLE_EXECUTABLE_NAME ${PROJECT_NAME}
            MACOSX_BUNDLE_BUNDLE_NAME "ADD Todo App"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.addtodoapp"
    )

    # Copy resources to bundle
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/resources
            COMMENT "Copying resources to app bundle"
            VERBATIM
    )

    # Copy font to the right location in bundle
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/Inter-VariableFont_opsz,wght.ttf
            $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/
            COMMENT "Copying font to app bundle"
            VERBATIM
    )

    # Copy sounds to bundle
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/sounds
            $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/sounds
            COMMENT "Copying resources to app bundle"
            VERBATIM
    )

    # Final bundle setup
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND chmod +x $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/MacOS/${PROJECT_NAME}
            COMMAND xattr -cr $<TARGET_BUNDLE_DIR:${PROJECT_NAME}> || true
            COMMAND echo "=== Bundle Contents ==="
            COMMAND find $<TARGET_BUNDLE_DIR:${PROJECT_NAME}> -type f | head -20
            COMMENT "Finalizing app bundle"
            VERBATIM
    )
else()
    # For non-bundle builds, copy resources to a local resources directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/resources
            ${CMAKE_CURRENT_BINARY_DIR}/resources
            COMMENT "Copying resources to build directory"
            VERBATIM
    )
endif()

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
        @ONLY
)
