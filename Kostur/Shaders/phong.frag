#version 330 core

struct Light{ 
	vec3 pos; 
	vec3 kA; 
	vec3 kD; 
	vec3 kS; 
};

struct Material{ 
	vec3 kA; 
	vec3 kD; 
	vec3 kS; 
	float shine; 
};

in vec3 chNor;
in vec3 chFragPos;
in vec2 chTex; // <--- Stigle koordinate

out vec4 outCol;

uniform Light uLight;
uniform Material uMaterial;
uniform vec3 uViewPos;

// NOVO:
uniform sampler2D texture_diffuse1; // Mora se zvati ovako zbog Mesh klase!
uniform int uUseTexture; // 0 = Iskljuceno, 1 = Ukljuceno

void main()
{
    // 1. Phong Osvetljenje (Sivo/Belo)
	vec3 resA = uLight.kA * uMaterial.kA;

	vec3 normal = normalize(chNor);
	vec3 lightDirection = normalize(uLight.pos - chFragPos);
	float nD = max(dot(normal, lightDirection), 0.0);
	vec3 resD = uLight.kD * (nD * uMaterial.kD);

	vec3 viewDirection = normalize(uViewPos - chFragPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float s = pow(max(dot(viewDirection, reflectionDirection), 0.0), uMaterial.shine);
	vec3 resS = uLight.kS * (s * uMaterial.kS);

    vec4 lightColor = vec4(resA + resD + resS, 1.0);

    // 2. Primena Teksture
    if(uUseTexture == 1) {
        vec4 texColor = texture(texture_diffuse1, chTex);
        outCol = lightColor * texColor; 
    } else {
        outCol = lightColor;
    }
}