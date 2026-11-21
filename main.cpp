#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <print>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

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

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;
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

[[nodiscard]] Mesh makeGrid(int cells, float size = 1.0f)
{
    const int side = cells + 1;
    assert(cells >= 1);
    assert(side * side - 1 <= std::numeric_limits<std::uint16_t>::max());

    Mesh mesh;
    mesh.vertices.reserve(side * side);
    mesh.indices.reserve(6 * cells * cells);

    for (int row = 0; row < side; ++row)
    {
        for (int column = 0; column < side; ++column)
        {
            const float u = static_cast<float>(column) / cells;
            const float v = static_cast<float>(row) / cells;

            mesh.vertices.push_back({
                .position = { (u - 0.5f) * size, (v - 0.5f) * size },
                .color = { 1.0f - u, u + v - 2.0f * u * v, u * v },
            });
        }
    }

    for (int row = 0; row < cells; ++row)
    {
        for (int column = 0; column < cells; ++column)
        {
            const auto bottomLeft = static_cast<std::uint16_t>(row * side + column);
            const auto bottomRight = static_cast<std::uint16_t>(bottomLeft + 1);
            const auto topLeft = static_cast<std::uint16_t>(bottomLeft + side);
            const auto topRight = static_cast<std::uint16_t>(topLeft + 1);

            mesh.indices.insert(mesh.indices.end(), {
                bottomLeft, bottomRight, topRight,
                bottomLeft, topRight, topLeft,
            });
        }
    }

    return mesh;
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

    constexpr int gridCells = 10;
    const Mesh grid = makeGrid(gridCells, 1.8f);

    const std::size_t vertexBytes = grid.vertices.size() * sizeof(Vertex);
    const std::size_t indexBytes = grid.indices.size() * sizeof(std::uint16_t);
    const std::size_t bytesWithoutIndices = grid.indices.size() * sizeof(Vertex);

    std::println("Grid {0}x{0}: {1} vertices, {2} indices", gridCells, grid.vertices.size(), grid.indices.size());
    std::println("With indices:    {} bytes", vertexBytes + indexBytes);
    std::println("Without indices: {} bytes", bytesWithoutIndices);

    const GLsizei indexCount = static_cast<GLsizei>(grid.indices.size());

    GLuint vbo = 0;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, vertexBytes, grid.vertices.data(), 0);

    GLuint ebo = 0;
    glCreateBuffers(1, &ebo);
    glNamedBufferStorage(ebo, indexBytes, grid.indices.data(), 0);

    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(vao, ebo);

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
        glPolygonMode(GL_FRONT_AND_BACK, glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS ? GL_LINE : GL_FILL);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, nullptr);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &ebo);
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
