#include <format>
#include <iostream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

// Ask laptop drivers to run this program on the discrete GPU.
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

const char* vertexShaderSource = R"(
    #version 460 core

    const vec2 positions[3] = vec2[3](
        vec2(-0.5, -0.5),
        vec2( 0.5, -0.5),
        vec2( 0.0,  0.5)
    );

    void main()
    {
        gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 460 core

    out vec4 color;

    void main()
    {
        color = vec4(1.0, 0.5, 0.2, 1.0);
    }
)";

GLuint compileShader(GLenum type, const char* source)
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
        std::cerr << std::format("Shader error: {}\n", message);
    }
    return shader;
}

GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader)
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
        std::cerr << std::format("Program error: {}\n", message);
    }
    return program;
}

int main()
{
    glfwSetErrorCallback([](int code, const char* description) {
        std::cerr << std::format("GLFW error {}: {}\n", code, description);
    });

    if (!glfwInit())
    {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Engine", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to load OpenGL functions\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::cout << std::format("OpenGL {}\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    std::cout << std::format("GPU    {}\n", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    // shader
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = linkProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.10f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
