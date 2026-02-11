#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

void main()
{
    FragPos = vec3(uM * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uM))) * aNormal;  
    TexCoords = aTexCoords;
    
    gl_Position = uP * uV * vec4(FragPos, 1.0);
}