#include <cstddef>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <unordered_set>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

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

[[nodiscard]] std::expected<std::string, std::string> readFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return std::unexpected(std::format("Cannot open {}", path.string()));
    }

    std::string text(std::filesystem::file_size(path), '\0');
    if (!file.read(text.data(), text.size()))
    {
        return std::unexpected(std::format("Cannot read {}", path.string()));
    }
    return text;
}

[[nodiscard]] GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char message[1024];
        glGetShaderInfoLog(shader, sizeof(message), nullptr, message);
        std::println(stderr, "Shader error: {}", message);
    }
    return shader;
}

[[nodiscard]] GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char message[1024];
        glGetProgramInfoLog(program, sizeof(message), nullptr, message);
        std::println(stderr, "Program error: {}", message);
    }
    return program;
}

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

int main()
{
    const std::filesystem::path shaderDir = std::filesystem::path(ASSETS_DIR).make_preferred() / "shaders";

    const auto vertexSource = readFile(shaderDir / "triangle.vert");
    const auto fragmentSource = readFile(shaderDir / "triangle.frag");

    if (!vertexSource)
    {
        std::println(stderr, "{}", vertexSource.error());
    }
    if (!fragmentSource)
    {
        std::println(stderr, "{}", fragmentSource.error());
    }
    if (!vertexSource || !fragmentSource)
    {
        return 1;
    }

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

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource->c_str());
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource->c_str());

    GLuint program = linkProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

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

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
