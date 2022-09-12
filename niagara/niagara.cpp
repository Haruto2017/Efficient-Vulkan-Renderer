// niagara.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <assert.h>
#include <iostream>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

int main()
{
    std::cout << "Hello World!\n";

    int rc = glfwInit();
    assert(rc);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "niagara", 0, 0);
    assert(window);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
}