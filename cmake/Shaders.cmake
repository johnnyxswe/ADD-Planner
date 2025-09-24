function(add_shaders TARGET_NAME)
    set(SHADER_SOURCE_FILES ${ARGN})
    list(LENGTH SHADER_SOURCE_FILES FILE_COUNT)
    if(FILE_COUNT EQUAL 0)
        message(FATAL_ERROR "Cannot add shaders target without shader files!")
    endif()

    set(SHADER_COMMANDS)
    set(SHADER_PRODUCTS)

    foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
        cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
        cmake_path(GET SHADER_SOURCE FILENAME SHADER_NAME)

        # COMMANDS
        list(APPEND SHADER_COMMANDS COMMAND)
        list(APPEND SHADER_COMMANDS Vulkan::glslc)
        list(APPEND SHADER_COMMANDS "${SHADER_SOURCE}")
        list(APPEND SHADER_COMMANDS "-o")
        list(APPEND SHADER_COMMANDS "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
        list(APPEND SHADER_PRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
    endforeach()

    add_custom_target(${TARGET_NAME} ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            ${SHADER_COMMANDS}
            COMMENT "Compiling Shaders..."
            SOURCES ${SHADER_SOURCE_FILES}
            BYPRODUCTS ${SHADER_PRODUCTS}
    )

    # Determine if we should create a bundle
    # Only create bundle for Release builds on macOS
    set(CREATE_BUNDLE FALSE)
    if(APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CREATE_BUNDLE TRUE)
    endif()


    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        # For macOS bundle: Copy compiled shaders to the bundle
        if(APPLE)
            # Create a custom command to copy compiled shaders after build
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                    $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/shaders
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_CURRENT_BINARY_DIR}/shaders
                    $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/shaders
                    COMMENT "Copying compiled shaders to app bundle"
            )
        endif()
    endif()
endfunction()