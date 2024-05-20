#include "OpenGL_Abstraction.h"

// Error Detection
void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}
void GLDetectError(const char* file, int line) {
    while (GLenum error = glGetError()) {
        std::cout << "[OpenGL Error] (" << error << "): " << file << ":" << line << std::endl;
    }
}

// Vertex Buffer
VertexBuffer::VertexBuffer(const void* data, unsigned int size)
{
    GLCall(glGenBuffers(1, &ID));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, ID));
    GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}
VertexBuffer::~VertexBuffer()
{
    GLCall(glDeleteBuffers(1, &ID));
}
void VertexBuffer::Bind()
{
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, ID));
}
void VertexBuffer::Unbind()
{
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
void VertexBuffer::SubData(float* data, unsigned int size)
{
    Bind();
    GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, size, (const void*)data));
    Unbind();
}
unsigned int VertexBuffer::GetID()
{
    return ID;
}

// Index Buffer
IndexBuffer::IndexBuffer(const unsigned int* data, unsigned int count)
    : Count(count)
{
    GLCall(glGenBuffers(1, &ID));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, Count * sizeof(unsigned int), data, GL_STATIC_DRAW));
}
IndexBuffer::~IndexBuffer()
{
    GLCall(glDeleteBuffers(1, &ID));
}
void IndexBuffer::Bind()
{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID));
}
void IndexBuffer::Unbind()
{
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}
unsigned int IndexBuffer::GetID()
{
    return ID;
}
unsigned int IndexBuffer::GetCount()
{
    return Count;
}
void IndexBuffer::SubData(unsigned int* data, unsigned int count)
{
    Bind();
    GLCall(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, count * sizeof(unsigned int), data));
    Unbind();
}

// Vertex Buffer Layout
VertexBufferLayout::VertexBufferLayout()
    : layoutSize(0)
{}
std::vector<LayoutElement> VertexBufferLayout::GetElements()
{
    return Elements;
}
unsigned int VertexBufferLayout::GetStride()
{
    return layoutSize;
}

// Vertex Array
VertexArray::VertexArray()
{
    GLCall(glGenVertexArrays(1, &ID));
}
VertexArray::~VertexArray()
{
    GLCall(glDeleteVertexArrays(1, &ID));
}
void VertexArray::AddBuffer(VertexBuffer* VBO, VertexBufferLayout* VBL)
{
    Bind();
    VBO->Bind();

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
void VertexArray::Bind()
{
    GLCall(glBindVertexArray(ID));
}
void VertexArray::Unbind()
{
    GLCall(glBindVertexArray(0));
}
unsigned int VertexArray::GetID()
{
    return ID;
}

// Shader
Shader::Shader(const std::string& filepath)
{
    std::pair<std::string, std::string> shaders = ParseShader(filepath);
    std::string vertexShader = shaders.first;
    std::string fragmentShader = shaders.second;

    ID = CreateShader(vertexShader, fragmentShader);
}
Shader::~Shader()
{
    GLCall(glDeleteProgram(ID));
}
std::pair<std::string, std::string> Shader::ParseShader(const std::string& filepath)
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
unsigned int Shader::CompileShader(unsigned int type, const std::string& source)
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
unsigned int Shader::CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
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
int Shader::GetUniformLocation(const std::string& name)
{
    if (uniformLocationCache.find(name) != uniformLocationCache.end())
        return uniformLocationCache[name];

    GLCall(int loc = glGetUniformLocation(ID, name.c_str()));
    if (loc == -1) std::cerr << "Warning: uniform  " << name << "doesn't exist" << std::endl;

    uniformLocationCache[name] = loc;
    std::cout << "Uniform " << name << " loc = " << loc << std::endl;
    return loc;
}
void Shader::Bind()
{
    GLCall(glUseProgram(ID));
}
void Shader::Unbind()
{
    GLCall(glUseProgram(0));
}
void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
}
void Shader::SetUniform3f(const std::string& name, float v0, float v1, float v2)
{
    GLCall(glUniform3f(GetUniformLocation(name), v0, v1, v2));
}
void Shader::SetUniformMatrix4fv(const std::string& name, const GLfloat* v)
{
    GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, v));
}

// Text
Text::Text(Shader* textshader)
    : textShader(textshader)
{
    // Initialize FreeType library
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not initialize FreeType Library" << std::endl;
        return;
    }

    // Load font face
    FT_Face face;
    if (FT_New_Face(ft, "resources/fonts/arial1.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    // Set font size
    FT_Set_Pixel_Sizes(face, 0, 48);
    GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    // Load characters
    for (unsigned char c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }

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

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store character for later use
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
Text::~Text()
{
    /*delete(VAO);
       delete(VBO);
       delete(VBL);
       delete(IBO);*/
}
void Text::RenderText(std::string text, float x, float y, float scale, glm::vec3 color)
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

// Renderer
void Renderer::Draw(VertexArray* vao, IndexBuffer* ibo, Shader* shader, const std::string& primitivetype)
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
void Renderer::Clear(float v1, float v2, float v3, float v4)
{
    glClearColor(v1, v2, v3, v4);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}