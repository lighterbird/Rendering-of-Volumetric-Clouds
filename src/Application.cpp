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
#include <random>
#include <string>
#include <sstream>
#include <fstream>
#include <ft2build.h>
#include <unordered_map>
#include FT_FREETYPE_H  
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h" 
#include "imgui/imgui_impl_opengl3.h" 

void GLClearError() { while (glGetError() != GL_NO_ERROR); }
void GLDetectError(const char* file, int line)
{
    GLenum errorCode = glGetError();
    if (errorCode != 0)
    {
        std::cerr << "GLerror at " << file << ":" << line << ": "
            << "Error Code " << errorCode << std::endl;
        exit(EXIT_FAILURE);
    }

}
#define GLCall(x) GLClearError();x;GLDetectError(__FILE__, __LINE__);
float PI = std::atan(45) * 4;

float Random()
{
    return (static_cast<float>(std::rand()) / RAND_MAX);
}

class VertexBuffer
{
private:
    unsigned int ID;
public:
    VertexBuffer(const void* data, unsigned int size)
    {
        // Initialize Vextex Buffer
        GLCall(glGenBuffers(1, &ID));
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, ID));
        GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
    }
    ~VertexBuffer()
    {
        // Delete VBO
        GLCall(glDeleteBuffers(1, &ID));
    }
    void Bind()
    {
        // Bind/Use VBO
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, ID));
    }
    void Unbind()
    {
        // Unbind VBO
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    }
    void SubData(float* data, unsigned int size)
    {
        // Substitute VBO data
        Bind();
        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, size, (const void*)data));
        Unbind();
    }
    unsigned int GetID()
    {
        return ID;
    }
};
class IndexBuffer
{
private:
    unsigned int ID;
    unsigned int Count;
public:
    IndexBuffer(const unsigned int* data, unsigned int count)
    {
        // No. of indices to draw
        Count = count;

        // Initialize IBO
        GLCall(glGenBuffers(1, &ID));
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID));
        GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, Count * sizeof(unsigned int), data, GL_STATIC_DRAW));
    }
    ~IndexBuffer()
    {
        // Delete IBO
        GLCall(glDeleteBuffers(1, &ID));
    }
    void Bind()
    {
        // Bind/Use IBO
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID));
    }
    void Unbind()
    {
        // Unbind IBO
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
    unsigned int GetID()
    {
        return ID;
    }
    unsigned int GetCount()
    {
        return Count;
    }
    void SubData(unsigned int* data, unsigned int count)
    {
        // Substitute IBO data
        Bind();
        GLCall(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, count * sizeof(unsigned int), data));
        Unbind();
    }

};
typedef struct
{
    unsigned int type;
    unsigned int count;
    unsigned char normalized;
    unsigned int typeSize;
}LayoutElement;
class VertexBufferLayout
{
private:
    std::vector<LayoutElement> Elements;
    unsigned int layoutSize;
public:
    VertexBufferLayout()
    {
        layoutSize = 0;
    }

    template<typename T>
    void Push(unsigned int count)
    {
        /*static_assert(false);*/
    }

    template <>
    void Push<float>(unsigned int count)
    {
        Elements.push_back({ GL_FLOAT, count, GL_FALSE, sizeof(float) });
        layoutSize += count * sizeof(float);
    }

    template <>
    void Push<int>(unsigned int count)
    {
        Elements.push_back({ GL_INT, count, GL_FALSE, sizeof(int) });
        layoutSize += count * sizeof(unsigned int);
    }

    template <>
    void Push<unsigned int>(unsigned int count)
    {
        Elements.push_back({ GL_UNSIGNED_INT, count, GL_FALSE, sizeof(unsigned int) });
        layoutSize += count * sizeof(int);
    }

    std::vector<LayoutElement> GetElements()
    {
        return Elements;
    }

    unsigned int GetStride()
    {
        return layoutSize;
    }
};
class VertexArray
{
private:
    unsigned int ID;
public:
    VertexArray()
    {
        // Initialize VAO
        GLCall(glGenVertexArrays(1, &ID));
    }
    ~VertexArray()
    {
        // Delete VAO
        GLCall(glDeleteVertexArrays(1, &ID));
    }
    void AddBuffer(VertexBuffer* VBO, VertexBufferLayout* VBL)
    {
        // Use VAO and VBO
        Bind();
        VBO->Bind();

        // Assign Layout to VAO
        std::vector<LayoutElement> elements = VBL->GetElements();
        unsigned int offSet = 0;
        for (unsigned int i = 0; i < elements.size(); i++)
        {
            LayoutElement element = elements[i];
            GLCall(glEnableVertexAttribArray(i));
            GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, VBL->GetStride(), (const void*)offSet));
            offSet += element.count * element.typeSize;
        }


    }
    void Bind()
    {
        // Bind/Use VAO
        GLCall(glBindVertexArray(ID));
    }
    void Unbind()
    {
        // Unbind VAO
        GLCall(glBindVertexArray(0));
    }
    unsigned int GetID()
    {
        return ID;
    }
};
class Shader
{
private:
    unsigned int ID;
    std::unordered_map<std::string, int> uniformLocationCache;

    static std::pair<std::string, std::string> ParseShader(const std::string& filepath)
    {
        std::ifstream stream(filepath);
        std::string line;
        std::stringstream ss[2];

        int NONE = -1, VERTEX = 0, FRAGMENT = 1;

        int shaderType = NONE;

        if (stream.is_open())
        {
            while (getline(stream, line))
            {
                if (line.find("#shader") != std::string::npos)
                {
                    if (line.find("vertex") != std::string::npos) shaderType = VERTEX;
                    else if (line.find("fragment") != std::string::npos) shaderType = FRAGMENT;

                }
                else
                {
                    if (shaderType != NONE) ss[shaderType] << line << "\n";
                }
            }
        }
        else
        {
            std::cout << "Failed to open File: " << filepath << std::endl;
        }


        std::pair<std::string, std::string> shaders = { ss[0].str(), ss[1].str() };
        return shaders;
    }
    static unsigned int CompileShader(unsigned int type, const std::string& source)
    {
        unsigned int id = glCreateShader(type);
        const char* src = source.c_str();
        GLCall(glShaderSource(id, 1, &src, nullptr));
        GLCall(glCompileShader(id));

        // Error Handling for Shader Compilation
        int result;
        GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result)); // i: int, v: vector (array)
        if (result == GL_FALSE)
        {
            int length;
            GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
            char* message = (char*)malloc(length * sizeof(char));
            GLCall(glGetShaderInfoLog(id, length, &length, message));
            std::cout << "Failed to Compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "Shader" << std::endl;
            std::cout << message << std::endl;
            GLCall(glDeleteShader(id));
            free(message);
            return 0;
        }

        return id;
    }
    static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
    {
        unsigned int program = glCreateProgram();
        unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
        unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        glValidateProgram(program);

        glDeleteShader(vs);
        glDeleteShader(fs);

        return program;
    }
    int GetUniformLocation(const std::string& name)
    {
        if (uniformLocationCache.find(name) != uniformLocationCache.end())
            return uniformLocationCache[name];

        GLCall(int loc = glGetUniformLocation(ID, name.c_str()));
        if (loc == -1) std::cerr << "Warning: uniform  " << name << "doesn't exist" << std::endl;

        uniformLocationCache[name] = loc;
        return loc;
    }

public:
    Shader(const std::string& filepath)
    {
        std::pair<std::string, std::string> shaders = ParseShader(filepath);
        std::string vertexShader = shaders.first;
        std::string fragmentShader = shaders.second;

        ID = CreateShader(vertexShader, fragmentShader);
    }
    ~Shader()
    {
        GLCall(glDeleteProgram(ID)); // Delete the shader
    }

    void Bind()
    {
        GLCall(glUseProgram(ID)); // Bind the shader
    }
    void Unbind()
    {
        GLCall(glUseProgram(0)); // Unbind the shader
    }

    void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
    {
        GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
    }
    void SetUniform3f(const std::string& name, float v0, float v1, float v2)
    {
        GLCall(glUniform3f(GetUniformLocation(name), v0, v1, v2));
    }
    void SetUniform1f(const std::string& name, float v0)
    {
        GLCall(glUniform1f(GetUniformLocation(name), v0));
    }
    void SetUniformMatrix4fv(const std::string& name, const GLfloat* v)
    {
        /*glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, );*/
        GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, v));
    }

};
typedef struct {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
}TextChar;
class Texture
{
private:
    unsigned int textureID;
    std::string filePath;
    bool Dim3;
    int height, width, bitsPerPixel;

    glm::vec3*** CreatePositions(int nX, int nY, int nZ)
    {
        glm::vec3 numCells = glm::vec3(nX, nY, nZ);
        glm::vec3*** positions = new glm::vec3 * *[numCells.x];
        for (int i = 0; i < numCells.x; ++i) {
            positions[i] = new glm::vec3 * [numCells.y];
            for (int j = 0; j < numCells.y; ++j) {
                positions[i][j] = new glm::vec3[numCells.z];
            }
        }

        glm::vec3 cellSize = 100.0f / numCells;

        for (int i = 0; i < numCells.x; i++)
        {
            for (int j = 0; j < numCells.y; j++)
            {
                for (int k = 0; k < numCells.z; k++)
                {
                    glm::vec3 pos;
                    if (Random() > 0.6f)
                    {
                        pos = glm::vec3(1000.0f, 1000.0f, 1000.0f);
                    }
                    else
                    {
                        glm::vec3 randomOffset = glm::vec3(Random(), Random(), Random());
                        pos = (glm::vec3(i, j, k) + randomOffset) * cellSize;
                    }
                    
                    positions[i][j][k] = pos;
                    //std::cout << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
                }
            }
        }

        return positions;
    }
    float* CreateWorley(int dimX, int dimY, int dimZ, int freq1, int freq2, int freq3, int freq4) // Dim is resolution of texture(number of disks in which the cell is being divided)
    {
        float* worleyNoise = new float[dimX * dimY * dimZ * 4];

        // Points positions array
        // Frequency is number of cells
        glm::vec3 numCells1 = glm::vec3(freq1, freq1, freq1);
        glm::vec3 numCells2 = glm::vec3(freq2, freq2, freq2);
        glm::vec3 numCells3 = glm::vec3(freq3, freq3, freq3);
        glm::vec3 numCells4 = glm::vec3(freq4, freq4, freq4);

        glm::vec3*** positions1 = CreatePositions(numCells1.x, numCells1.y, numCells1.z);
        glm::vec3*** positions2 = CreatePositions(numCells2.x, numCells2.y, numCells2.z);
        glm::vec3*** positions3 = CreatePositions(numCells3.x, numCells3.y, numCells3.z);
        glm::vec3*** positions4 = CreatePositions(numCells4.x, numCells4.y, numCells4.z);

        // Define 3D texture
        
        for (int channel = 0; channel <= 3; channel++)
        {
            float maxDist = -1.0f;
            glm::vec3 numCells;
            glm::vec3*** positions = nullptr;
            if (channel == 0)
            {
                positions = positions1;
                numCells = numCells1;
            }
            if (channel == 1)
            {
                positions = positions2;
                numCells = numCells2;
            }
            if (channel == 2)
            {
                positions = positions3;
                numCells = numCells3;
            }
            if (channel == 3)
            {
                positions = positions4;
                numCells = numCells4;
            }

            for (int i = 0; i < dimX; i++)
            {
                for (int j = 0; j < dimY; j++)
                {
                    for (int k = 0; k < dimZ; k++)
                    {
                        glm::vec3 voxelPos = 100.0f * glm::vec3((float)(i + 0.5f) / (float)dimX, (float)(j + 0.5f) / (float)dimY, (float)(k + 0.5f) / (float)dimZ);

                        int cellIndexX = (int)(voxelPos.x * numCells.x) / 100.0f;
                        int cellIndexY = (int)(voxelPos.y * numCells.y) / 100.0f;
                        int cellIndexZ = (int)(voxelPos.z * numCells.z) / 100.0f;

                        float minDist = 1000.0f;

                        for (int ii = -1; ii <= 1; ii++)
                        {
                            for (int jj = -1; jj <= 1; jj++)
                            {
                                for (int kk = -1; kk <= 1; kk++)
                                {
                                    int queryIndexX = cellIndexX + ii;
                                    int queryIndexY = cellIndexY + jj;
                                    int queryIndexZ = cellIndexZ + kk;

                                    bool xLow = false, xHigh = false, yLow = false, yHigh = false, zLow = false, zHigh = false;

                                    if (queryIndexX < 0) { queryIndexX = numCells.x - 1; xLow = true; }
                                    if (queryIndexX >= numCells.x) { queryIndexX = 0; xHigh = true; }

                                    if (queryIndexY < 0) { queryIndexY = numCells.y - 1; yLow = true; }
                                    if (queryIndexY >= numCells.y) { queryIndexY = 0; yHigh = true; }

                                    if (queryIndexZ < 0) { queryIndexZ = numCells.z - 1; zLow = true; }
                                    if (queryIndexZ >= numCells.z) { queryIndexZ = 0; zHigh = true; }

                                    glm::vec3 queryPos = positions[queryIndexX][queryIndexY][queryIndexZ];

                                    if (xLow) queryPos.x -= 100.0f;
                                    if (yLow) queryPos.y -= 100.0f;
                                    if (zLow) queryPos.z -= 100.0f;

                                    if (xHigh) queryPos.x += 100.0f;
                                    if (yHigh) queryPos.y += 100.0f;
                                    if (zHigh) queryPos.z += 100.0f;

                                    glm::vec3 dis = voxelPos - queryPos;
                                    float dist = glm::dot(dis, dis);

                                    if (dist < minDist) minDist = dist;

                                }
                            }
                        }
                        if (minDist > maxDist) maxDist = minDist; // For normalization
                        worleyNoise[channel + 4 * (i + dimX * (j + dimY * k))] = minDist;
                    }
                }
            }

            // Normalize distance
            for (int i = 0; i < dimX; i++)
            {
                for (int j = 0; j < dimY; j++)
                {
                    for (int k = 0; k < dimZ; k++)
                    {
                        worleyNoise[channel + 4 * (i + dimX * (j + dimY * k))] = 1.0f - ((float)worleyNoise[channel + 4 * (i + dimX * (j + dimY * k))] / (float)maxDist);
                        /*if (worleyNoise[channel + 3 * (i + dimX * (j + dimY * k))] < 0.7f)
                            worleyNoise[channel + 3 * (i + dimX * (j + dimY * k))] = 0.0f;*/
                        //std::cout << worleyNoise[i + dimX * (j + dimY * (k + dimZ * channel))] << std::endl;
                    }
                }
            }

            for (int i = 0; i < numCells.x; ++i) {
                for (int j = 0; j < numCells.y; ++j) {
                    delete[] positions[i][j];
                }
                delete[] positions[i];
            }
            delete[] positions;
        }
        
        return worleyNoise;
    }
public:
    Texture(const std::string& path, const bool dim3)
        : textureID(0), filePath(path), Dim3(dim3), height(0), width(0), bitsPerPixel(0)
    {
        if (!dim3)
        {
            glGenTextures(1, &textureID);
            stbi_set_flip_vertically_on_load(1);
            unsigned char* localBuffer = stbi_load(filePath.c_str(), &width, &height, &bitsPerPixel, 4);

            // std::cout << filePath << " " << width << ", " << height << ", " << bitsPerPixel << std::endl;
            if (!localBuffer) {
                std::cerr << "Failed to load texture: " << filePath << std::endl;
                return;
            }

            
            glBindTexture(GL_TEXTURE_2D, textureID);

            // Set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localBuffer);
            stbi_image_free(localBuffer);

            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            

            int res = 64;
            const int dimX = 64, dimY = 64, dimZ = 64;
            float* worleyNoise = CreateWorley(dimX, dimY, dimZ, 4, 8, 16, 32);

            // Create and bind a 3D texture
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_3D, textureID);

            // Set texture parameters
            GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT));
            GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT));
            GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT));
            GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

            // Upload the Worley noise data to the 3D texture
            GLCall(glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, dimX, dimY, dimZ, 0, GL_RGBA, GL_FLOAT, (void*)worleyNoise));

            // Unbind the texture
            GLCall(glBindTexture(GL_TEXTURE_3D, 0));

            delete[] worleyNoise;

            
        }
        
    }
    ~Texture()
    {
        glDeleteTextures(1, &textureID);
    }
    void Bind()
    {
        glActiveTexture(GL_TEXTURE0);
        if(Dim3) glBindTexture(GL_TEXTURE_3D, textureID);
        else glBindTexture(GL_TEXTURE_2D, textureID);
    }
    void Unbind()
    {
        if (Dim3) glBindTexture(GL_TEXTURE_3D, 0);
        else glBindTexture(GL_TEXTURE_2D, 0);
        
    }
};
class Text
{
private:
    std::map<char, TextChar> textchars;
    /*VertexBuffer* VBO;
    VertexBufferLayout* VBL;
    VertexArray* VAO;
    IndexBuffer* IBO;*/
    Shader* textShader;

public:
    Text(Shader* textshader)
    {
        textShader = textshader;
        /*VAO = new VertexArray();*/

        /*float vertices[] = {1.0f ,2.0f ,3.0f ,4.0f};
        unsigned int indices[] = {1,2,3};

        VBO = new VertexBuffer(vertices, sizeof(vertices));
        IBO = new IndexBuffer(indices, sizeof(indices));

        VBL = new VertexBufferLayout();
        VBL->Push<float>(4);

        VAO->AddBuffer(VBO, VBL);*/

        FT_Library ft;
        if (FT_Init_FreeType(&ft))
        {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        }

        FT_Face face;
        if (FT_New_Face(ft, "resources/fonts/arial1.ttf", 0, &face))
        {
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, 48);
        GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1)); // disable byte-alignment restriction

        for (unsigned char c = 0; c < 128; c++)
        {
            // load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // now store character for later use
            TextChar textchar = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                face->glyph->advance.x
            };
            textchars.insert(std::pair<char, TextChar>(c, textchar));
        }
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }
    ~Text()
    {
        /*delete(VAO);
        delete(VBO);
        delete(VBL);
        delete(IBO);*/
    }
    void RenderText(std::string text, float x, float y, float scale, glm::vec3 color)
    {
        unsigned int VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        textShader->Bind();
        textShader->SetUniform3f("textColor", color.x, color.y, color.z);
        glBindVertexArray(VAO);

        // iterate through all characters
        std::string::const_iterator c;
        for (c = text.begin(); c != text.end(); c++)
        {
            TextChar ch = textchars[*c];

            float xpos = x + ch.Bearing.x * scale;
            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

            float w = ch.Size.x * scale;
            float h = ch.Size.y * scale;
            // update VBO for each character
            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };
            // render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            // update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            // render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        //glActiveTexture(GL_TEXTURE0);
        //VAO->Bind();

        //// iterate through all characters
        //std::string::const_iterator c;
        //for (c = text.begin(); c != text.end(); c++)
        //{
        //    TextChar ch = textchars[*c];

        //    float xpos = x + ch.Bearing.x * scale;
        //    float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        //    float w = ch.Size.x * scale;
        //    float h = ch.Size.y * scale;
        //    // update VBO for each character

        //    float vertices[] = {
        //        xpos, ypos + h, 0.0f, 0.0f,
        //        xpos, ypos, 0.0f, 1.0f,
        //        xpos + w, ypos, 1.0f, 1.0f,
        //        xpos + w, ypos + h, 1.0f, 0.0f
        //    };

        //    unsigned int indices[] = {
        //        0,1,2,
        //        0,3,2
        //    };

        //    // render glyph texture over quad
        //    glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        //    // update content of memory
        //    VBO->Bind();
        //    GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices));
        //    VBO->Unbind();

        //    IBO->Bind();
        //    GLCall(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices));
        //    IBO->Unbind();

        //    // render quad
        //    VAO->Bind();
        //    IBO->Bind();
        //    GLCall(glDrawElements(GL_TRIANGLES, IBO->GetCount(), GL_UNSIGNED_INT, nullptr));

        //    // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        //    x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
        //}
        //VAO->Unbind();
        //glBindTexture(GL_TEXTURE_2D, 0);
    }
};
class Renderer
{
private:
public:
    void Draw(VertexArray* vao, IndexBuffer* ibo, Shader* shader, const std::string& primitivetype)
    {
        shader->Bind();
        vao->Bind();
        ibo->Bind();

        if (primitivetype == "triangles")
        {
            GLCall(glDrawElements(GL_TRIANGLES, ibo->GetCount(), GL_UNSIGNED_INT, nullptr));
        }
        else if (primitivetype == "points")
        {
            GLCall(glDrawElements(GL_POINTS, ibo->GetCount(), GL_UNSIGNED_INT, nullptr));
        }

    }
    void Clear(float v1, float v2, float v3, float v4)
    {
        glClearColor(v1, v2, v3, v4);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
};

class App;
class Camera
{
private:
    int height;
    int width;

    std::vector<Shader*> shaders;
    App* appState;
public:
    glm::vec3 camPosition;
    glm::vec3 targetPoint;
    int view;
    float r, theta, phi;

    Camera(App* app);
    void Update();
};

class Light
{
private:
    VertexArray* VAO;
    VertexBuffer* VBO;
    VertexBufferLayout* VBL;

    std::vector <IndexBuffer*> IBOs;
    Shader* shader;
    Texture* texture;
    Renderer* renderer;

    App* appState;

    std::string objname;
    
public:
    glm::vec3 lightPosition;
    glm::vec3 lightColour;
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec3 Accelaration;
    glm::vec3 Rotation;
    glm::vec3 Scaling;

    bool lightVisible;

    Light(App* app);
    ~Light()
    {

    }
    std::vector<int> GetIndex(std::string indexString, bool texture = false)
    {
        if (texture)
        {
            //std::cout << "Entered GetIndex with texture = true" << std::endl;
            std::vector<int> indices;
            std::stringstream ss(indexString);
            std::string token;
            while (std::getline(ss, token, '/'))
            {
                indices.push_back(std::stoi(token));
            }
            // for (int i = 0; i < indices.size();i++) std::cout << indices[i] << " ";
            return indices;
        }
        else
        {
            size_t pos = indexString.find("//");

            // Check if "//" is found in the string
            if (pos != std::string::npos)
            {
                std::string firstIntStr = indexString.substr(0, pos);
                std::string secondIntStr = indexString.substr(pos + 2);

                // Convert the substrings to integers
                int firstInt = std::stoi(firstIntStr);
                int secondInt = std::stoi(secondIntStr);

                return { firstInt,secondInt };
            }
            return { 0,0 };
        }



    }
    std::pair<std::vector<float>, std::vector<unsigned int>> ReadOBJFile(const std::string& filePath, int indicesOffset = 0, bool texture = false)
    {
        //std::cout << "Reading OBJ" << std::endl;
        std::vector<float> vertices = {};
        std::vector<unsigned int> indices = {};

        std::vector<glm::vec3> tempVerts = {};
        std::vector<glm::vec3> tempNormals = {};
        std::vector<glm::vec2> tempTex = {};

        std::fstream file;
        file.open(filePath, std::ios::in);

        if (file.is_open())
        {
            std::string line; // Each line one by one of the OBJ
            int ind = 0;

            while (getline(file, line))
            {
                //std::cout << line << std::endl;
                // Seperate the line into a vector of words
                std::istringstream iss(line);
                std::vector<std::string> words;
                do
                {
                    std::string word;
                    iss >> word;
                    words.push_back(word);
                } while (iss);

                if (words[0] == "v")
                {
                    //std::cout << "v: " << std::endl;
                    tempVerts.push_back(glm::vec3(stof(words[1]), stof(words[2]), stof(words[3])));
                }
                if (words[0] == "vn")
                {
                    //std::cout << "vn: " << std::endl;
                    tempNormals.push_back(glm::vec3(stof(words[1]), stof(words[2]), stof(words[3])));
                }
                if (words[0] == "vt" && texture == true)
                {
                    //std::cout << "vt: " << std::endl;
                    tempTex.push_back(glm::vec2(stof(words[1]), stof(words[2])));
                }
                if (words[0] == "f")
                {
                    for (int i = 1; i < 4; i++)
                    {
                        std::vector<int> index = GetIndex(words[i], texture);
                        unsigned int vIndex = index[0];
                        unsigned int nIndex, tIndex;
                        if (texture)
                        {
                            tIndex = index[1];
                            nIndex = index[2];
                        }
                        else
                        {
                            nIndex = index[1];
                        }

                        vertices.push_back(tempVerts[vIndex - 1].x);
                        vertices.push_back(tempVerts[vIndex - 1].y);
                        vertices.push_back(tempVerts[vIndex - 1].z);

                        vertices.push_back(tempNormals[nIndex - 1].x);
                        vertices.push_back(tempNormals[nIndex - 1].y);
                        vertices.push_back(tempNormals[nIndex - 1].z);

                        if (texture)
                        {
                            vertices.push_back(tempTex[tIndex - 1].x);
                            vertices.push_back(tempTex[tIndex - 1].y);
                        }
                    }

                    indices.push_back(ind + indicesOffset);
                    indices.push_back(ind + indicesOffset + 1);
                    indices.push_back(ind + indicesOffset + 2);

                    ind += 3;
                }
            }
            file.close();
            return { vertices, indices };
        }
        std::cerr << "Could not open file " << filePath << std::endl;
    }
    void LightSpecifics();
    void Draw();
    void Update();
};
class Object 
{
private:
    VertexArray* VAO;
    VertexBuffer* VBO;
    VertexBufferLayout* VBL;

    std::vector <IndexBuffer*> IBOs;
    Shader* shader;
    Texture* texture;
    Renderer* renderer;

    App* appState;

    std::string objname;

public:
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec3 Accelaration;
    glm::vec3 Rotation;
    glm::vec3 Scaling;

    glm::vec3 surfaceColor;
    glm::vec3 surfaceDiffuse;
    glm::vec3 surfaceSpec;

    float n_samples;
    float n_lightSamples;
    float maxDensity;
    float falloff;
    float cloudScale;
    bool play;
    float shadow_samples;

    glm::vec3 cloudOffset;

    Object(App* app, std::string objname);
    ~Object()
    {
        // Delete VBO, VAO, IBO
        delete(VAO);
        delete(VBO);
        delete(VBL);

        for (int i = 0; i < IBOs.size(); i++)
        {
            delete(IBOs[i]);
        }
    }

    void Draw();
    void Update();
    std::vector<int> GetIndex(std::string indexString, bool texture = false)
    {
        if (texture)
        {
            //std::cout << "Entered GetIndex with texture = true" << std::endl;
            std::vector<int> indices;
            std::stringstream ss(indexString);
            std::string token;
            while (std::getline(ss, token, '/'))
            {
                indices.push_back(std::stoi(token));
            }
            // for (int i = 0; i < indices.size();i++) std::cout << indices[i] << " ";
            return indices;
        }
        else
        {
            size_t pos = indexString.find("//");

            // Check if "//" is found in the string
            if (pos != std::string::npos)
            {
                std::string firstIntStr = indexString.substr(0, pos);
                std::string secondIntStr = indexString.substr(pos + 2);

                // Convert the substrings to integers
                int firstInt = std::stoi(firstIntStr);
                int secondInt = std::stoi(secondIntStr);

                return { firstInt,secondInt };
            }
            return { 0,0 };
        }



    }
    std::pair<std::vector<float>, std::vector<unsigned int>> ReadOBJFile(const std::string& filePath, int indicesOffset = 0, bool texture = false)
    {
        //std::cout << "Reading OBJ" << std::endl;
        std::vector<float> vertices = {};
        std::vector<unsigned int> indices = {};

        std::vector<glm::vec3> tempVerts = {};
        std::vector<glm::vec3> tempNormals = {};
        std::vector<glm::vec2> tempTex = {};

        std::fstream file;
        file.open(filePath, std::ios::in);

        if (file.is_open())
        {
            std::string line; // Each line one by one of the OBJ
            int ind = 0;

            while (getline(file, line))
            {
                //std::cout << line << std::endl;
                // Seperate the line into a vector of words
                std::istringstream iss(line);
                std::vector<std::string> words;
                do
                {
                    std::string word;
                    iss >> word;
                    words.push_back(word);
                } while (iss);

                if (words[0] == "v")
                {
                    //std::cout << "v: " << std::endl;
                    tempVerts.push_back(glm::vec3(stof(words[1]), stof(words[2]), stof(words[3])));
                }
                if (words[0] == "vn")
                {
                    //std::cout << "vn: " << std::endl;
                    tempNormals.push_back(glm::vec3(stof(words[1]), stof(words[2]), stof(words[3])));
                }
                if (words[0] == "vt" && texture == true)
                {
                    //std::cout << "vt: " << std::endl;
                    tempTex.push_back(glm::vec2(stof(words[1]), stof(words[2])));
                }
                if (words[0] == "f")
                {
                    for (int i = 1; i < 4; i++)
                    {
                        std::vector<int> index = GetIndex(words[i], texture);
                        unsigned int vIndex = index[0];
                        unsigned int nIndex, tIndex;
                        if (texture)
                        {
                            tIndex = index[1];
                            nIndex = index[2];
                        }
                        else
                        {
                            nIndex = index[1];
                        }

                        vertices.push_back(tempVerts[vIndex - 1].x);
                        vertices.push_back(tempVerts[vIndex - 1].y);
                        vertices.push_back(tempVerts[vIndex - 1].z);

                        vertices.push_back(tempNormals[nIndex - 1].x);
                        vertices.push_back(tempNormals[nIndex - 1].y);
                        vertices.push_back(tempNormals[nIndex - 1].z);

                        if (texture)
                        {
                            vertices.push_back(tempTex[tIndex - 1].x);
                            vertices.push_back(tempTex[tIndex - 1].y);
                        }
                    }

                    indices.push_back(ind + indicesOffset);
                    indices.push_back(ind + indicesOffset + 1);
                    indices.push_back(ind + indicesOffset + 2);

                    ind += 3;
                }
            }
            file.close();
            return { vertices, indices };
        }
        std::cerr << "Could not open file " << filePath << std::endl;
    }
    void ObjectSpecifics();
};

class App 
{
private:

public:
    GLFWwindow* window;
    Renderer* renderer;
    Camera* camera;
    Text* text;
    std::vector<Shader*> shaders;
    ImGuiIO io;

    std::vector<Object*> objects;
    Light* light;

    int height;
    int width;
    float deltaTime;

    App()
    {
        // Seed Random Generator
        std::srand(static_cast<unsigned int>(std::time(nullptr)));

        // Setup OpenGL and Imgui
        height = 1000, width = 1920;
        window = OpenGLInit(width, height);
        ImGUIInit();

        // Create renderer
        renderer = new Renderer;

        // Create Shaders
        shaders.push_back(new Shader("resources/shaders/textshader.glsl"));
        shaders.push_back(new Shader("resources/shaders/shader_1.glsl"));
        shaders.push_back(new Shader("resources/shaders/shader_cloud.glsl"));
        shaders.push_back(new Shader("resources/shaders/shader_light.glsl"));

        // Create Camera;
        camera = new Camera(this);

        // Create Text Renderer
        text = new Text(shaders[0]);

        SceneInit();
        MainLoop();
    }
    ~App()
    {
        delete(camera);  
        delete(text);
        delete(renderer);
        for (int i = 0; i < shaders.size(); i++) delete(shaders[i]);

        for (int i = 0; i < objects.size(); i++) delete(objects[i]);
        delete(light);

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
    GLFWwindow* OpenGLInit(unsigned int width, unsigned int height)
    {
        // Set up GLFW
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Create window
        GLFWwindow* window0 = glfwCreateWindow(width, height, "Clouds", NULL, NULL);
        if (!window0)
        {
            glfwTerminate();
            return nullptr;
        }
        glfwSetWindowPos(window0, 0, 40);

        // Create context
        glfwMakeContextCurrent(window0);

        // Turn on VSync (limit FPS to Monitor Refresh Rate)
        //glfwSwapInterval(1);


        // Set up GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return nullptr;
        }

        // Enable depth buffer
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        /*glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_SUBTRACT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        

        // Define the viewport dimensions
        glViewport(0, 0, height, height);

        return window0;
    }
    void SceneInit()
    {
        objects.push_back(new Object(this, "plane"));
        objects.push_back(new Object(this, "cloud"));

        light = new Light(this);
        light->lightColour = glm::vec3(1.0f, 1.0f, 1.0f);
    }
    void ImGUIInit()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }
    void DrawUI()
    {
        ImGui::SetNextWindowPos(ImVec2(height, 0));
        ImGui::SetNextWindowSize(ImVec2(width-height, 200));
        ImGui::Begin("Light Settings");
        ImGui::SliderFloat("LightPos.x", &light->lightPosition.x, -50.0f, 50.0f);
        ImGui::SliderFloat("LightPos.y", &light->lightPosition.y, -50.0f, 50.0f);
        ImGui::SliderFloat("LightPos.z", &light->lightPosition.z, -50.0f, 500.0f);
        ImGui::SliderFloat("LightColour.r", &light->lightColour.x, 0.0f, 1.0f);
        ImGui::SliderFloat("LightColour.g", &light->lightColour.y, 0.0f, 1.0f);
        ImGui::SliderFloat("LightColour.b", &light->lightColour.z, 0.0f, 1.0f);
        ImGui::Checkbox("light source visible", &light->lightVisible);
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(height, 200));
        ImGui::SetNextWindowSize(ImVec2(width - height, 290));
        ImGui::Begin("Ground Settings");
        ImGui::SliderFloat("Ground Scale.x", &objects[0]->Scaling.x, 0.0f, 100.0f);
        ImGui::SliderFloat("Ground Scale.y", &objects[0]->Scaling.y, 0.0f, 100.0f);
        ImGui::SliderFloat("Ground Scale.z", &objects[0]->Scaling.z, 0.0f, 100.0f);
        ImGui::SliderFloat("GroundColor.r", &objects[0]->surfaceColor.x, 0.0f, 1.0f);
        ImGui::SliderFloat("GroundColor.g", &objects[0]->surfaceColor.y, 0.0f, 1.0f);
        ImGui::SliderFloat("GroundColor.b", &objects[0]->surfaceColor.z, 0.0f, 1.0f);
        ImGui::SliderFloat("SurfaceDiffuse", &objects[0]->surfaceDiffuse.x, 0.0f, 1.0f);
        objects[0]->surfaceDiffuse.y = objects[0]->surfaceDiffuse.x;
        objects[0]->surfaceDiffuse.z = objects[0]->surfaceDiffuse.x;
        ImGui::SliderFloat("SurfaceSpec", &objects[0]->surfaceSpec.x, 0.0f, 1.0f);
        ImGui::SliderFloat("shadow_samples", &objects[0]->shadow_samples, 0.0f, 200.0f);
        objects[0]->surfaceSpec.y = objects[0]->surfaceSpec.x;
        objects[0]->surfaceSpec.z = objects[0]->surfaceSpec.x;
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(height, 490));
        ImGui::SetNextWindowSize(ImVec2(width - height, 125));
        ImGui::Begin("Camera Settings");
        ImGui::SliderFloat("CameraPosition.r", &camera->r, 0.0f, 100.0f);
        ImGui::SliderFloat("CameraPosition.theta", &camera->theta, 0, 2 * PI);
        ImGui::SliderFloat("CameraPosition.phi", &camera->phi, 0, 2 * PI);

        camera->camPosition.x = camera->r * std::cos(camera->phi) * std::sin(camera->theta);
		camera->camPosition.y = camera->r * std::sin(camera->phi) * std::sin(camera->theta);
		camera->camPosition.z = camera->r * std::cos(camera->theta);

        ImGui::Text("FPS: %.3f", 1 / deltaTime);
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(height, 615));
        ImGui::SetNextWindowSize(ImVec2(width - height, height - 615));
        ImGui::Begin("Cloud Settings");
        ImGui::SliderFloat("n_samples", &objects[1]->n_samples, 0.0f, 200.0f);
        ImGui::SliderFloat("n_lightSamples", &objects[1]->n_lightSamples, 0.0f, 200.0f);
        ImGui::SliderFloat("Max Density", &objects[1]->maxDensity, 0.0f, 100.0f);
        //ImGui::SliderFloat("Fall Off", &objects[1]->falloff, 0.0f, 10.0f);
        ImGui::SliderFloat("Cloud Scale", &objects[1]->cloudScale, 0.0f, 2.0f);
        ImGui::SliderFloat("Box Scale.x", &objects[1]->Scaling.x, 0.0f, 10.0f);
        ImGui::SliderFloat("Box Scale.y", &objects[1]->Scaling.y, 0.0f, 10.0f);
        ImGui::SliderFloat("Box Scale.z", &objects[1]->Scaling.z, 0.0f, 10.0f);
        ImGui::SliderFloat("Box Position.x", &objects[1]->Position.x, -10.0f, 10.0f);
        ImGui::SliderFloat("Box Position.y", &objects[1]->Position.y, -10.0f, 10.0f);
        ImGui::SliderFloat("Box Position.z", &objects[1]->Position.z, -10.0f, 10.0f);
        ImGui::SliderFloat("Cloud offset.x", &objects[1]->cloudOffset.x, -10.0f, 10.0f);
        ImGui::SliderFloat("Cloud offset.y", &objects[1]->cloudOffset.y, -10.0f, 10.0f);
        ImGui::SliderFloat("Cloud offset.z", &objects[1]->cloudOffset.z, -10.0f, 10.0f);
        if (ImGui::Button("Play")) {
            objects[1]->play = true;
        }
        if (ImGui::Button("Pause")) {
            objects[1]->play = false;
        }
        //ImGui::SliderFloat("CloudColour.r", &objects[1]->surfaceColor.x, 0.0f, 1.0f);
        //ImGui::SliderFloat("CloudColour.g", &objects[1]->surfaceColor.y, 0.0f, 1.0f);
        //ImGui::SliderFloat("CloudColour.b", &objects[1]->surfaceColor.z, 0.0f, 1.0f);
        //ImGui::SliderFloat("SurfaceDiffuse", &objects[1]->surfaceDiffuse.x, 0.0f, 1.0f);
        objects[1]->surfaceDiffuse.y = objects[1]->surfaceDiffuse.x;
        objects[1]->surfaceDiffuse.z = objects[1]->surfaceDiffuse.x;
        //ImGui::SliderFloat("SurfaceSpec", &objects[1]->surfaceSpec.x, 0.0f, 1.0f);
        objects[1]->surfaceSpec.y = objects[1]->surfaceSpec.x;
        objects[1]->surfaceSpec.z = objects[1]->surfaceSpec.x;
        ImGui::End();


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    void Update()
    {
        camera->Update();
        light->Update();
        for (int i = 0; i < objects.size(); i++) objects[i]->Update();
    }
    void Draw()
    {
        light->Draw();
        for (int i = 0; i < objects.size(); i++) objects[i]->Draw();
    }
    void MainLoop() 
    {
        float prevTime = glfwGetTime();
        while (!glfwWindowShouldClose(window))
        {
            float currTime = glfwGetTime(); deltaTime = currTime - prevTime; prevTime = currTime;
            renderer->Clear(120.0f / 255.0f, 196.0f / 255.0f, 253.0f / 255.0f, 1.0f);
            //renderer->Clear(0.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 1.0f);

            Update();
            Draw();
            DrawUI();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    
};

Object::Object(App* app, std::string objName)
{
    appState = app;
    renderer = appState->renderer;
    objname = objName;

    // Initialize object attributes
    Position = glm::vec3(0.0f, 0.0f, 0.0f);
    Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    Scaling = glm::vec3(1.0f, 1.0f, 1.0f);

    ObjectSpecifics();
}
void Object::ObjectSpecifics()
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    if (objname == "plane")
    {
        shader = appState->shaders[1];
        //texture = new Texture("resources/textures/start_line.jpg");
        /*vertices = {
            -1.0, -1.0, 0.0f, 0.0f, 0.0f, 1.0f,
             1.0, -1.0, 0.0f, 0.0f, 0.0f, 1.0f,
             1.0,  1.0, 0.0f, 0.0f, 0.0f, 1.0f,
            -1.0,  1.0, 0.0f, 0.0f, 0.0f, 1.0f
        };
        indices = {
            0,1,2,
            0,3,2
        };*/

        std::pair<std::vector<float>, std::vector<unsigned int>> p = ReadOBJFile("resources/models/plane.obj", 0, false);
        vertices = p.first;
        indices = p.second;

        VBL = new VertexBufferLayout();
        VBL->Push<float>(3);
        VBL->Push<float>(3);

        surfaceColor = glm::vec3(10.0f / 255.0f, 200.0f / 255.0f, 10.0f / 255.0f);
        surfaceDiffuse = glm::vec3(0.8f, 0.8f, 0.8f);
        surfaceSpec = glm::vec3(0.7f, 0.7f, 0.7f);
        Scaling = glm::vec3(11.375f, 14.124f, 1.0f);

        shadow_samples = 25.0f;

    }
    if (objname == "cloud")
    {
        shader = appState->shaders[2];
        std::pair<std::vector<float>, std::vector<unsigned int>> p = ReadOBJFile("resources/models/cube.obj", 0, false);
        vertices = p.first;
        indices = p.second;
        
        int i = 0;
        std::cout << "vertices : [" << std::endl;
        while (i < vertices.size())
        {
            std::cout << "(" << vertices[i] << ", " << vertices[i + 1] << ", " << vertices[i + 2]
                << ", " << vertices[i + 3] << ", " << vertices[i + 4] << ", " << vertices[i + 5] << ")" << std::endl;
            i += 6;
        }
        std::cout << "]" << std::endl;
        std::cout << "Indices Size: " << indices.size() << std::endl;
        std::cout << "indices : [" << std::endl;

        i = 0;
        while (i < indices.size())
        {
            std::cout << "(" << indices[i] << ", " << indices[i + 1] << ", " << indices[i + 2]
                << ")" << std::endl;
            i += 3;
        }
        std::cout << "]" << std::endl;

        VBL = new VertexBufferLayout();
        VBL->Push<float>(3);
        VBL->Push<float>(3);
        
        surfaceColor = glm::vec3(1.0f, 1.0f, 1.0f);
        n_samples = 20;
        n_lightSamples = 8;
        maxDensity = 8.926f;
        falloff = 0.048f;
        cloudScale = 0.048f;
        Scaling = glm::vec3(10.0f, 10.0f, 1.498f);
        Position = glm::vec3(0.0f, 0.0f, 8.42f);
        cloudOffset = glm::vec3(0.0f, 0.0f, 0.0f);
        play = true;

        texture = new Texture("", true);
    }

    VBO = new VertexBuffer(vertices.data(), vertices.size() * sizeof(float));
    VAO = new VertexArray();
    VAO->AddBuffer(VBO, VBL);

    IBOs.push_back(new IndexBuffer(indices.data(), indices.size()));

    

    
}
void Object::Draw()
{
    // Define Model Matrix
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), Position);
    glm::mat4 rotateX = glm::rotate(glm::mat4(1.0f), Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotateY = glm::rotate(glm::mat4(1.0f), Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0f), Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scaling);

    glm::mat4 RotationMatrix = scale * rotateY * rotateZ * rotateX;
    glm::mat4 ModelMatrix = translate * RotationMatrix;


    // Set  Uniforms
    shader->Bind();
    shader->SetUniformMatrix4fv("modelMatrix", &ModelMatrix[0][0]);

    if (objname == "plane")
    {
        glm::mat4 translate1 = glm::translate(glm::mat4(1.0f), appState->objects[1]->Position);
        glm::mat4 rotateX1 = glm::rotate(glm::mat4(1.0f), appState->objects[1]->Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotateY1 = glm::rotate(glm::mat4(1.0f), appState->objects[1]->Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotateZ1 = glm::rotate(glm::mat4(1.0f), appState->objects[1]->Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 scale1 = glm::scale(glm::mat4(1.0f), appState->objects[1]->Scaling);

        glm::mat4 RotationMatrix1 = scale1 * rotateY1 * rotateZ1 * rotateX1;
        glm::mat4 ModelMatrix1 = translate1 * RotationMatrix1;

        appState->objects[1]->texture->Bind();
        shader->SetUniformMatrix4fv("cloudModelMatrix", &ModelMatrix1[0][0]);
        shader->SetUniform3f("cubeMinInitial", -1.0f, -1.0f, -1.0f);
        shader->SetUniform3f("cubeMaxInitial", 1.0f, 1.0f, 1.0f);
        shader->SetUniform1f("shadow_samples", shadow_samples);
        shader->SetUniform1f("maxDensity", appState->objects[1]->maxDensity);
        shader->SetUniform1f("cloudScale", appState->objects[1]->cloudScale);
        shader->SetUniform3f("cloudOffset", appState->objects[1]->cloudOffset.x, appState->objects[1]->cloudOffset.y, appState->objects[1]->cloudOffset.z);
        
        shader->SetUniform3f("surfaceColour", surfaceColor.x, surfaceColor.y, surfaceColor.z);
        shader->SetUniform3f("surfaceDiffuseCoefficient", surfaceDiffuse.x, surfaceDiffuse.x, surfaceDiffuse.x);
        shader->SetUniform3f("surfaceSpecularCoefficient", surfaceSpec.x, surfaceSpec.x, surfaceSpec.x);

        renderer->Draw(VAO, IBOs[0], shader, "triangles");
    }
    if (objname == "cloud")
    {
        //shader->SetUniformMatrix4fv("rotationMatrix", &ModelMatrix[0][0]);
        texture->Bind();
        shader->SetUniform3f("cubeMinInitial", -1.0f, -1.0f, -1.0f);
        shader->SetUniform3f("cubeMaxInitial",  1.0f,  1.0f,  1.0f);
        shader->SetUniform1f("n_samples", n_samples);
        shader->SetUniform1f("n_lightSamples", n_lightSamples);
        shader->SetUniform1f("maxDensity", maxDensity);
        shader->SetUniform1f("falloff", falloff);
        shader->SetUniform1f("cloudScale", cloudScale);
        shader->SetUniform3f("cloudOffset", cloudOffset.x, cloudOffset.y, cloudOffset.z);
        shader->SetUniform3f("surfaceColour", surfaceColor.x, surfaceColor.y, surfaceColor.z);
        shader->SetUniform3f("surfaceDiffuseCoefficient", surfaceDiffuse.x, surfaceDiffuse.x, surfaceDiffuse.x);
        shader->SetUniform3f("surfaceSpecularCoefficient", surfaceSpec.x, surfaceSpec.x, surfaceSpec.x);

        renderer->Draw(VAO, IBOs[0], shader, "triangles");
    }
}
void Object::Update()
{
    if (objname == "cloud")
    {
        if (play) {
            cloudOffset.y = cloudOffset.y - appState->deltaTime * 0.8f;
            cloudOffset.x = cloudOffset.x + appState->deltaTime * 0.4f;
        }
    }
}

Light::Light(App* app)
{
    appState = app;
    lightPosition = glm::vec3(-6.00f, 0.0f, 24.448f);
    lightColour = glm::vec3(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
    LightSpecifics();
}
void Light::LightSpecifics()
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    shader = appState->shaders[3];

    std::pair<std::vector<float>, std::vector<unsigned int>> p = ReadOBJFile("resources/models/light.obj", 0, false);
    vertices = p.first;
    indices = p.second;

    VBL = new VertexBufferLayout();
    VBL->Push<float>(3);
    VBL->Push<float>(3);

    Scaling = glm::vec3(1.0f, 1.0f, 1.0f);

    VBO = new VertexBuffer(vertices.data(), vertices.size() * sizeof(float));
    VAO = new VertexArray();
    VAO->AddBuffer(VBO, VBL);

    IBOs.push_back(new IndexBuffer(indices.data(), indices.size()));
}
void Light::Draw()
{
    for (int i = 1; i < appState->shaders.size() - 1; i++)
    {
        appState->shaders[i]->Bind();
        appState->shaders[i]->SetUniform3f("lightPosition", lightPosition.x, lightPosition.y, lightPosition.z);
        appState->shaders[i]->SetUniform4f("lightColor", lightColour.x, lightColour.y, lightColour.z, 1.0f);
    }

    // Define Model Matrix
    Position = lightPosition;
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), Position);
    glm::mat4 rotateX = glm::rotate(glm::mat4(1.0f), Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotateY = glm::rotate(glm::mat4(1.0f), Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0f), Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scaling);

    glm::mat4 RotationMatrix = scale * rotateY * rotateZ * rotateX;
    glm::mat4 ModelMatrix = translate * RotationMatrix;


    // Set  Uniforms
    shader->Bind();
    shader->SetUniformMatrix4fv("modelMatrix", &ModelMatrix[0][0]);
    shader->SetUniform4f("lightColor", lightColour.x, lightColour.y, lightColour.z, 1.0f);

    if (lightVisible) {
        renderer->Draw(VAO, IBOs[0], shader, "triangles");
    }
    
}
void Light::Update()
{

}

Camera::Camera(App* app)
{
    // Set App
    appState = app;

    // Set aspect ratio
    height = appState->height;
    width = appState->width;

    width = height;
    shaders = appState->shaders;

    // Set initial cam parameters
    camPosition = glm::vec3(10.0f, 10.0f, 10.0f);
    targetPoint = glm::vec3(0.0f, 0.0f, 0.0f);

    r = 41.409f;
    theta = 1.192f;
    phi = 0.0f;
}
void Camera::Update()
{
    if (view == 0)
    {
        targetPoint = glm::vec3(0.0f, 0.0f, 0.0f);
        //camPosition = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::mat4 viewMatrix = glm::lookAt(camPosition, targetPoint, up);
    glm::mat4 projectionMatrix = glm::perspective(45.0f, (float)height / width, 0.1f, 1000.0f);
    glm::mat4 camMatrix = projectionMatrix * viewMatrix;

    // Set uniforms
    for (int i = 1; i < shaders.size(); i++)
    {
        shaders[i]->Bind();
        shaders[i]->SetUniformMatrix4fv("cameraMatrix", &camMatrix[0][0]);
        if (i != 3) {
            shaders[i]->SetUniform3f("cameraPosition", camPosition.x, camPosition.y, camPosition.z);
        }
    }
}

int main(void)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    App app;
    return 0;
}
