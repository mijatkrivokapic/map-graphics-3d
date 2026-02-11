#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
uniform int uIsCircle;
void main()
{
    if (uIsCircle == 1) 
    {
        vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
        if (dot(circCoord, circCoord) > 1.0) {
            discard;
        }
    }
    FragColor = uColor;
}