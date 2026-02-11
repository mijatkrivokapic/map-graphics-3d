#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uMapTexture;
uniform vec2 uOffset;
uniform float uZoom;

void main()
{
    vec2 zoomedCoord = (TexCoord * uZoom) + uOffset;

    if(zoomedCoord.x < 0.0 || zoomedCoord.x > 1.0 || zoomedCoord.y < 0.0 || zoomedCoord.y > 1.0)
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    else
        FragColor = texture(uMapTexture, zoomedCoord);
}