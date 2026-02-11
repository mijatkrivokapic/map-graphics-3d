#version 330 core
out vec4 FragColor;

// ---------------- STRUCT DEFINICIJE ----------------
struct Material {
    vec3 kA; // Ambijentalna boja
    vec3 kD; // Difuzna boja
    vec3 kS; // Spekularna boja
    float shine;
};

struct DirLight { // Sunce
    vec3 direction; // Smer svetla (ne pozicija!)
    
    vec3 kA;
    vec3 kD;
    vec3 kS;
};

struct PointLight { // Ciode
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 kA;
    vec3 kD;
    vec3 kS;
};

#define NR_POINT_LIGHTS 32

// ---------------- INPUT PROMENLJIVE ----------------
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// ---------------- UNIFORMS ----------------
uniform vec3 uViewPos;
uniform Material uMaterial;

uniform DirLight uSun; // Preimenovali smo u 'uSun' da bude jasnije
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform int uNrActiveLights;

uniform sampler2D uTexture; // Tekstura
uniform int uUseTexture;    // Da li koristimo teksturu?

// ---------------- FUNKCIJE ----------------

// 1. Racunanje Sunca (Directional Light)
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shine);
    
    // Combine
    vec3 ambient = light.kA * uMaterial.kA;
    vec3 diffuse = light.kD * (diff * uMaterial.kD);
    vec3 specular = light.kS * (spec * uMaterial.kS);
    return (ambient + diffuse + specular);
}

// 2. Racunanje Cioda (Point Light)
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shine);
    
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // Combine
    vec3 ambient = light.kA * uMaterial.kA;
    vec3 diffuse = light.kD * (diff * uMaterial.kD);
    vec3 specular = light.kS * (spec * uMaterial.kS);
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

void main()
{
    // Properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uViewPos - FragPos);
    
    // 1. Kreni od crne boje
    vec3 result = vec3(0.0);

    // 2. Dodaj Sunce
    result += CalcDirLight(uSun, norm, viewDir);

    // 3. Dodaj Ciode
    for(int i = 0; i < uNrActiveLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    // 4. Primeni Teksturu (Mnozenje svetla sa bojom piksela)
    vec4 texColor = vec4(1.0); // Podrazumevana bela ako nema teksture
    if(uUseTexture == 1) {
        texColor = texture(uTexture, TexCoords);
    }
    
    FragColor = vec4(result, 1.0) * texColor;
}