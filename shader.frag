#version 420 core
out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct DirLight {
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 diffuse;
    vec3 specular;

    float PLfarPlane;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;

    vec3 diffuse;
    vec3 specular;       
};

#define MAX_LIGHTS 20
#define NUM_CASCADES 4

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[MAX_LIGHTS];
uniform int numPointLights;
uniform SpotLight spotLights[MAX_LIGHTS];
uniform int numSpotLights;
uniform Material material;
uniform vec3 ambient;

uniform sampler2DArray DLshadowMap;
uniform float DLfarPlane;

uniform sampler2DArray PLshadowMapArray;




layout (std140, binding = 1) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1

uniform samplerCube skybox;

// function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, unsigned int index);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

float DLShadowCalculation(vec3 fragPosWorldSpace)
{
    // Select cascade layer
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    // Transform fragment position into light space
    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    
    // Apply normal bias: push fragment along normal in light space
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(-dirLight.direction);
    
    // Control strength of normal bias
    float normalBiasStrength = 0.01; 
    vec3 normalBias = normal * normalBiasStrength;

    // Recalculate position in light space with normal bias
    vec4 biasedFragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace + normalBias, 1.0);
    vec3 projCoords = biasedFragPosLightSpace.xyz / biasedFragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // If outside shadow map region, return no shadow
    if (projCoords.z > 1.0)
    {
        return 0.0;
    }

    // Compute depth bias based on angle
    float angle = max(dot(normal, lightDir), 0.0);
    float bias = max(0.005 * (1.0 - angle), 0.0005);

    // Adjust bias based on cascade level
    const float biasModifier = 0.05;
    if (layer == cascadeCount)
    {
        bias *= 1 / (DLfarPlane * biasModifier);
    }
    else
    {
        bias *= 1 / (cascadePlaneDistances[layer] * biasModifier);
    }

    // Apply PCF (Percentage Closer Filtering)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(DLshadowMap, 0));
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(DLshadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}
//
//float PLShadowCalculation(vec3 fragPos, PointLight light, unsigned int index)
//{
//    // get vector between fragment position and light position
//    vec3 fragToLight = fragPos - light.position;
//    // ise the fragment to light vector to sample from the depth map    
//    float closestDepth = texture(shadowMap, fragToLight).r;
//    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
//    closestDepth *= light.PLfarPlane;
//    // now get current linear depth as the length between the fragment and light position
//    float currentDepth = length(fragToLight);
//    // test for shadows
//    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
//    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;        
//    // display closestDepth as debug (to visualize depth cubemap)
//     //FragColor = vec4(vec3(closestDepth / PLfarPlane), 1.0);    
//        
//    return shadow;
//}
//

float PLShadowCalculation(vec3 fragPos, PointLight light, uint lightIndex)
{
    // Get vector between fragment position and light position
    vec3 fragToLight = fragPos - light.position;

    // Determine which face of the cube the fragment falls into
    int faceIndex;
    vec2 uv;

    vec3 absFragToLight = abs(fragToLight);

    if (absFragToLight.x > absFragToLight.y && absFragToLight.x > absFragToLight.z) {
        faceIndex = fragToLight.x > 0 ? 0 : 1; // +X or -X
        uv = vec2(-fragToLight.z, -fragToLight.y) / absFragToLight.x;
    } else if (absFragToLight.y > absFragToLight.x && absFragToLight.y > absFragToLight.z) {
        faceIndex = fragToLight.y > 0 ? 2 : 3; // +Y or -Y
        uv = vec2(fragToLight.x, fragToLight.z) / absFragToLight.y;
    } else {
        faceIndex = fragToLight.z > 0 ? 4 : 5; // +Z or -Z
        uv = vec2(fragToLight.x, -fragToLight.y) / absFragToLight.z;
    }

    // Convert UV from [-1, 1] to [0, 1]
    uv = uv * 0.5 + 0.5;

    // Compute the correct layer in the 2D texture array
    unsigned int layerIndex = (lightIndex * 6) + faceIndex;

    // Sample depth from 2D array
    float closestDepth = texture(PLshadowMapArray, vec3(uv, layerIndex)).r;

    // Transform the depth value back to the original range [0, farPlane]
    closestDepth *= light.PLfarPlane;

    // Calculate the current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);

    // Apply a bias to reduce shadow acne
    float bias = 0.05; // Adjust this value as needed

    // Test for shadows
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}


//float PLShadowCalculation(vec3 fragPos, PointLight light, unsigned int index)
//{
//    // Get vector between fragment position and light position
//    vec3 fragToLight = fragPos - light.position;
//
//    // Sample from the cube map array using the fragment-to-light vector and the layer index
//    float closestDepth = texture(PLshadowMapArray, vec4(fragToLight, index)).r;
//
//    // Transform the depth value back to the original range [0, farPlane]
//    closestDepth *= light.PLfarPlane;
//
//    // Calculate the current linear depth as the length between the fragment and light position
//    float currentDepth = length(fragToLight);
//
//    // Apply a bias to reduce shadow acne
//    float bias = 0.05; // Adjust this value as needed
//
//    // Test for shadows
//    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
//
//    return shadow;
//}

void main()
{    
    // properties
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    // phase 1: directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // phase 2: point lights
    for(unsigned int i = 0; i < numPointLights; i++){
        result += CalcPointLight(pointLights[i], norm, fs_in.FragPos, viewDir, i); 
    }

    //result += CalcPointLight(pointLights[0], norm, fs_in.FragPos, viewDir, PLshadowMap[0], pointLights[0].PLfarPlane); 
    //result += CalcPointLight(pointLights[1], norm, fs_in.FragPos, viewDir, PLshadowMap[1], pointLights[1].PLfarPlane); 
    //result += CalcPointLight(pointLights[2], norm, fs_in.FragPos, viewDir, PLshadowMap[2], pointLights[2].PLfarPlane); 
    
    // phase 3: spot light
    for(int i = 0; i < numSpotLights; i++)
        result += CalcSpotLight(spotLights[i], norm, fs_in.FragPos, viewDir); 
        
    result += ambient * vec3(texture(material.texture_diffuse1, fs_in.TexCoords)) * material.diffuse;
    
    FragColor = vec4(result, 1.0);
}


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    // combine results

    float shadow = DLShadowCalculation(fs_in.FragPos);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, fs_in.TexCoords)) * material.diffuse;
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, fs_in.TexCoords)) * material.specular;
    return (1.0 - shadow) * (diffuse + specular);
}

 //calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, unsigned int index){
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance);    
    // combine results
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, fs_in.TexCoords)) * material.diffuse;
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, fs_in.TexCoords)) * material.specular;
    diffuse *= attenuation;
    specular *= attenuation;
    float shadow = PLShadowCalculation(fragPos, light, index);
         
    return (1.0 - shadow) * (diffuse + specular);
}
// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance);    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, fs_in.TexCoords)) * material.diffuse;
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, fs_in.TexCoords)) * material.specular;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (diffuse + specular);
}
