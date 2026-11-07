#include <format>
#include <iostream>

#include <GLFW/glfw3.h>


int main()
{
    std::cout << std::format("GLFW {}\n", glfwGetVersionString());
    return 0;
}
