#shader vertex
#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormalCoord;

uniform mat4 modelMatrix;
uniform mat4 cameraMatrix;

void main()
{
    vec3 currentPosition = vec3(modelMatrix * vec4(vertexPosition, 1.0f));
    gl_Position = cameraMatrix * vec4(currentPosition, 1.0f);
}


#shader fragment

#version 330 core

out vec4 color;

uniform vec4 lightColor;

void main()
{
    color = lightColor;
}