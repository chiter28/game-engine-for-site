#include <cstddef>
#include <filesystem>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <unordered_set>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "Shader.h"

// Ask laptop drivers to run this program on the discrete GPU.
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

struct Vertex
{
    float position[2];
    float color[3];
};

[[nodiscard]] std::string_view severityName(GLenum severity)
{
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         return "HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM:       return "MEDIUM";
        case GL_DEBUG_SEVERITY_LOW:          return "LOW";
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
        default:                             return "UNKNOWN";
    }
}

void APIENTRY onOpenGLMessage(
    GLenum /*source*/,
    GLenum /*type*/,
    GLuint /*id*/,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* /*userParam*/)
{
    static std::unordered_set<std::string> shown;
    if (!shown.emplace(message, length).second)
    {
        return;
    }

    std::println(stderr, "OpenGL [{}]: {}", severityName(severity), std::string_view(message, length));
}

[[nodiscard]] bool run(GLFWwindow* window)
{
    const auto shaderDir = std::filesystem::path(ASSETS_DIR).make_preferred() / "shaders";

    const auto shader = Shader::FromFiles(shaderDir / "triangle.vert", shaderDir / "triangle.frag");
    if (!shader)
    {
        std::println(stderr, "{}", shader.error());
        return false;
    }

    const Vertex vertices[] = {
        { .position = { -0.5f, -0.5f }, .color = { 1.0f, 0.0f, 0.0f } },
        { .position = {  0.5f, -0.5f }, .color = { 0.0f, 1.0f, 0.0f } },
        { .position = {  0.0f,  0.5f }, .color = { 0.0f, 0.0f, 1.0f } },
    };

    const GLsizei vertexCount = static_cast<GLsizei>(std::size(vertices));

    GLuint vbo = 0;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, sizeof(vertices), vertices, 0);

    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(vao, 0, 0);

    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, color));
    glVertexArrayAttribBinding(vao, 1, 0);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.10f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader->Use();
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    return true;
}

int main()
{
    glfwSetErrorCallback([](int code, const char* description) {
        std::println(stderr, "GLFW error {}: {}", code, description);
    });

    if (!glfwInit())
    {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Engine", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::println(stderr, "Failed to load OpenGL functions");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(onOpenGLMessage, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    std::println("OpenGL {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    std::println("GPU    {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    const bool success = run(window);

    glfwDestroyWindow(window);
    glfwTerminate();
    return success ? 0 : 1;
}
