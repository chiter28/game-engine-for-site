#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include <glad/gl.h>

class Shader
{
public:
    [[nodiscard]] static std::expected<Shader, std::string> FromFiles(
        const std::filesystem::path& vertexPath,
        const std::filesystem::path& fragmentPath);

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    ~Shader();

    void Use() const;

private:
    explicit Shader(GLuint program);

    GLuint m_Program = 0;
};
