#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#include <map>
#include <ctime>
#include <string>
#include <sstream>
#include <fstream>
#include <ft2build.h>
#include <unordered_map>
#include FT_FREETYPE_H  

void GLClearError();
void GLDetectError(const char* file, int line);
#define GLCall(x) GLClearError();x;GLDetectError(__FILE__, __LINE__);

class VertexBuffer
{
private:
    unsigned int ID;
public:
    VertexBuffer(const void* data, unsigned int size);
    ~VertexBuffer();
    void Bind();
    void Unbind();
    void SubData(float* data, unsigned int size);
    unsigned int GetID();
};

class IndexBuffer
{
private:
    unsigned int ID;
    unsigned int Count;
public:
    IndexBuffer(const unsigned int* data, unsigned int count);
    ~IndexBuffer();
    void Bind();
    void Unbind();
    unsigned int GetID();
    unsigned int GetCount();
    void SubData(unsigned int* data, unsigned int count);
};

typedef struct
{
    unsigned int type;
    unsigned int count;
    unsigned char normalized;
    unsigned int typeSize;
} LayoutElement;

class VertexBufferLayout
{
private:
    std::vector<LayoutElement> Elements;
    unsigned int layoutSize;
public:
    VertexBufferLayout();
    template<typename T>
    void Push(unsigned int count);
    std::vector<LayoutElement> GetElements();
    unsigned int GetStride();
};

class VertexArray
{
private:
    unsigned int ID;
public:
    VertexArray();
    ~VertexArray();
    void AddBuffer(VertexBuffer* VBO, VertexBufferLayout* VBL);
    void Bind();
    void Unbind();
    unsigned int GetID();
};

class Shader
{
private:
    unsigned int ID;
    std::unordered_map<std::string, int> uniformLocationCache;
    static std::pair<std::string, std::string> ParseShader(const std::string& filepath);
    static unsigned int CompileShader(unsigned int type, const std::string& source);
    static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
    int GetUniformLocation(const std::string& name);
public:
    Shader(const std::string& filepath);
    ~Shader();
    void Bind();
    void Unbind();
    void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
    void SetUniform3f(const std::string& name, float v0, float v1, float v2);
    void SetUniformMatrix4fv(const std::string& name, const GLfloat* v);
};

typedef struct {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
} TextChar;

class Text
{
private:
    std::map<char, TextChar> textchars;
    Shader* textShader;
public:
    Text(Shader* textshader);
    ~Text();
    void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);
};

class Renderer
{
private:
public:
    void Draw(VertexArray* vao, IndexBuffer* ibo, Shader* shader, const std::string& primitivetype);
    void Clear(float v1, float v2, float v3, float v4);
};
