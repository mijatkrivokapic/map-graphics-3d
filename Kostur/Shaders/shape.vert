#version 330 core
layout (location = 0) in vec2 aPos;

void main()
{
    // Pretvaramo koordinate (0.0 do 1.0) u OpenGL koordinate (-1.0 do 1.0)
    // Formula: x * 2 - 1
    gl_Position = vec4(aPos.x * 2.0 - 1.0, aPos.y * 2.0 - 1.0, 0.0, 1.0);
    
    gl_PointSize = 20.0; 
}