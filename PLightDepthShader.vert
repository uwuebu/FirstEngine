#version 460 core
layout (location = 0) in vec3 aPos; // Vertex position in model space

uniform mat4 model;           // Model matrix
//uniform mat4 lightSpaceMatrix; // Light's view-projection matrix

//out vec4 FragPos;

void main()
{
    // Transform the vertex position into light clip space
    //FragPos = model * vec4(aPos, 1.0);
    gl_Position = /* lightSpaceMatrix * */ model * vec4(aPos, 1.0);
}

