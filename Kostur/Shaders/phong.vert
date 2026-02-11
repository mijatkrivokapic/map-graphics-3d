#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNor;
layout(location = 2) in vec2 inTex; // <--- NOVO: Teksturne koordinate

out vec3 chFragPos;
out vec3 chNor;
out vec2 chTex; // <--- Saljemo ih u Fragment shader

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

void main()
{
	chFragPos = vec3(uM * vec4(inPos, 1.0));
	gl_Position = uP * uV * vec4(chFragPos, 1.0);
	
	chNor = mat3(transpose(inverse(uM))) * inNor; 
    chTex = inTex; // <--- Prosledi dalje
}