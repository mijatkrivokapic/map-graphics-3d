#version 330 core
out vec4 FragColor;

// ---------------- STRUCT DEFINITIONS ----------------
struct Material {
    vec3 kA;    // Ambient color
    vec3 kD;    // Diffuse color
    vec3 kS;    // Specular color
    float shine;
};

struct DirLight { // Sun
    vec3 direction; // Light direction (not position!)
    
    vec3 kA;
    vec3 kD;
    vec3 kS;
};

struct PointLight { // Pin lights
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 kA;
    vec3 kD;
    vec3 kS;
};

#define NR_POINT_LIGHTS 32

// ---------------- INPUT VARIABLES ----------------
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// ---------------- UNIFORMS ----------------
uniform vec3 uViewPos;
uniform Material uMaterial;

uniform DirLight uSun; // Renamed to 'uSun' for clarity
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform int uNrActiveLights;

uniform sampler2D uTexture; // Texture
uniform int uUseTexture;    // Should we use the texture?

// ---------------- FUNCTIONS ----------------

// 1. Calculate Sun (Directional Light)
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

// 2. Calculate Pin Lights (Point Light)
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
    
    // 1. Start with black color (accumulator)
    vec3 result = vec3(0.0);

    // 2. Add Sun contribution
    result += CalcDirLight(uSun, norm, viewDir);

    // 3. Add Pin Lights contribution
    for(int i = 0; i < uNrActiveLights; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    // 4. Apply Texture (Multiply light result with pixel color)
    vec4 texColor = vec4(1.0); // Default white if no texture is present
    if(uUseTexture == 1) {
        texColor = texture(uTexture, TexCoords);
    }
    
    FragColor = vec4(result, 1.0) * texColor;
}