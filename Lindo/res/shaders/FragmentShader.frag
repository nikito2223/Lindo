#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
    bool useTexture;
    vec3 color;
};

struct DirLight {
    vec3 direction;
    vec3 color;
    float intensity;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;  // Добавьте это поле
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

#define NR_POINT_LIGHTS 1

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform float shadowBias; // передавайте из программы, например, 0.001–0.005

// Тени (упрощаем до одной карты для направленного света)
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform bool shadowsEnabled;

// Debug режим
uniform bool debugMode;
uniform bool showLightIcons;
uniform float lightIconRadius;

// Прототипы функций
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec4 fragPosLightSpace);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

vec3 getDiffuseColor() {
    return material.useTexture ? vec3(texture(material.diffuse, TexCoords)) : material.color;
}

vec3 getSpecularColor() {
    if (material.useTexture)
        return vec3(texture(material.specular, TexCoords));
    else
        return vec3(1.0); // или можно ввести отдельный specularColor в Material
}

void main()
{    
    // ==== Рендер иконок источников света ====
    if (debugMode && showLightIcons) {
        for(int i = 0; i < NR_POINT_LIGHTS; i++) {
            if (pointLights[i].enabled) {
                vec3 toLight = pointLights[i].position - FragPos;
                float distToLight = length(toLight);
                
                if (distToLight < lightIconRadius) {
                    float intensity = 1.0 - distToLight / lightIconRadius;
                    vec3 baseColor = pointLights[i].color * pointLights[i].intensity;
                    float alpha = mix(0.3, 0.9, intensity);
                    FragColor = vec4(baseColor, alpha);
                    return;
                }
            }
        }
        
        if (spotLight.enabled) {
            vec3 toLight = spotLight.position - FragPos;
            float distToLight = length(toLight);
            
            if (distToLight < lightIconRadius) {
                float intensity = 1.0 - distToLight / lightIconRadius;
                vec3 baseColor = spotLight.color * spotLight.intensity;
                float alpha = mix(0.3, 0.9, intensity);
                FragColor = vec4(baseColor, alpha);
                return;
            }
        }
    }

    // Получаем базовый цвет материала
    vec3 materialColor;
    if (material.useTexture) {
        materialColor = vec3(texture(material.diffuse, TexCoords));
    } else {
        materialColor = material.color;
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Фаза 1: направленное освещение
    vec3 result = CalcDirLight(dirLight, norm, viewDir, FragPosLightSpace);
    
    // Фаза 2: точечные источники
    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }
    
    // Фаза 3: прожектор
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    
    FragColor = vec4(result, 1.0);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    if (!shadowsEnabled) return 0.0;
    
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0) return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  
    
    
    // Простой PCF 3x3
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec4 fragPosLightSpace)
{
    if (!light.enabled) return vec3(0.0);
    
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * light.color * light.intensity * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    
    if (shadowsEnabled) {
        float shadow = ShadowCalculation(fragPosLightSpace, normal, lightDir);
        diffuse *= (1.0 - shadow);
        specular *= (1.0 - shadow);
    }
    
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    if (!light.enabled) return vec3(0.0);
    
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    
    // Проверка на радиус действия света
    
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * light.color * light.intensity * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    if (!light.enabled) return vec3(0.0);
    
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * light.color * light.intensity * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}