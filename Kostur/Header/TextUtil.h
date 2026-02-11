#pragma once
#include <string>

void initText(unsigned int shaderProgram, const char* fontPath);
void RenderText(unsigned int shader, std::string text, float x, float y, float scale, float r, float g, float b);

struct Character {
    unsigned int TextureID;
    int          Size[2];
    int          Bearing[2];
    unsigned int Advance;
};
