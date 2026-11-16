#include "Shader.h"

#include <format>
#include <fstream>
#include <utility>

namespace
{
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

    [[nodiscard]] std::string readInfoLog(GLuint object, PFNGLGETSHADERIVPROC getParameter, PFNGLGETSHADERINFOLOGPROC getLog)
    {
        GLint logLength = 0;
        getParameter(object, GL_INFO_LOG_LENGTH, &logLength);

        std::string log(logLength, '\0');
        GLsizei written = 0;
        getLog(object, logLength, &written, log.data());
        log.resize(written);
        if (log.ends_with('\n'))
        {
            log.pop_back();
        }
        return log;
    }

    [[nodiscard]] std::expected<GLuint, std::string> compileShader(GLenum type, const std::filesystem::path& path)
    {
        const auto source = readFile(path);
        if (!source)
        {
            return std::unexpected(source.error());
        }

        const GLuint shader = glCreateShader(type);
        const GLchar* text = source->c_str();
        const auto length = static_cast<GLint>(source->size());
        glShaderSource(shader, 1, &text, &length);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            const std::string log = readInfoLog(shader, glGetShaderiv, glGetShaderInfoLog);
            glDeleteShader(shader);
            return std::unexpected(std::format("Cannot compile {}:\n{}", path.string(), log));
        }

        return shader;
    }
}

std::expected<Shader, std::string> Shader::FromFiles(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath)
{
    const auto vertexShader = compileShader(GL_VERTEX_SHADER, vertexPath);
    const auto fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentPath);
    if (!vertexShader || !fragmentShader)
    {
        glDeleteShader(vertexShader.value_or(0));
        glDeleteShader(fragmentShader.value_or(0));

        const char* separator = (!vertexShader && !fragmentShader) ? "\n" : "";
        return std::unexpected(std::format("{}{}{}", vertexShader.error_or(""), separator, fragmentShader.error_or("")));
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, *vertexShader);
    glAttachShader(program, *fragmentShader);
    glLinkProgram(program);

    glDeleteShader(*vertexShader);
    glDeleteShader(*fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        const std::string log = readInfoLog(program, glGetProgramiv, glGetProgramInfoLog);
        glDeleteProgram(program);
        return std::unexpected(std::format("Cannot link {} and {}:\n{}", vertexPath.string(), fragmentPath.string(), log));
    }

    return Shader(program);
}

Shader::Shader(GLuint program)
    : m_Program(program)
{
}

Shader::Shader(Shader&& other) noexcept
    : m_Program(std::exchange(other.m_Program, 0))
{
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        glDeleteProgram(m_Program);
        m_Program = std::exchange(other.m_Program, 0);
    }
    return *this;
}

Shader::~Shader()
{
    glDeleteProgram(m_Program);
}

void Shader::Use() const
{
    glUseProgram(m_Program);
}
