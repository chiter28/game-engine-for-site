#include <print>

#include <GLFW/glfw3.h>


int main()
{
    glfwSetErrorCallback([](int code, const char* description) {
        std::println(stderr, "GLFW error {}: {}", code, description);
    });

    if (!glfwInit())
    {
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Engine", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
