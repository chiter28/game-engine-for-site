#include <print>

#include <GLFW/glfw3.h>


int main()
{
    std::println("GLFW {}", glfwGetVersionString());
    return 0;
}
