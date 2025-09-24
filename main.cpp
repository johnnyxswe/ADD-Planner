//
// Created by Johnny Gonzales on 8/19/25.
//
#include <filesystem>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "spdlog/spdlog.h"
#include "src/application.h"


int main()
{
    try
    {
        todo::Application app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
