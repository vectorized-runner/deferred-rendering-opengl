#version 410 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

const int maxLightCount = 256;

uniform vec3 lightPositions[maxLightCount];
uniform vec3 lightIntensities[maxLightCount];
uniform vec3 cameraPos;
uniform int lightCount;

vec3 Iamb = vec3(0.8, 0.8, 0.8); // ambient light intensity
vec3 ka = vec3(0.3, 0.3, 0.3);   // ambient reflectance coefficient

float distancesq(vec3 a, vec3 b){
    vec3 diff = a - b;
    return dot(diff, diff);
}

void main()
{
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    vec3 Specular = vec3(texture(gAlbedoSpec, TexCoords).a);
    
    vec3 totalDiffuse = vec3(0, 0, 0);
    vec3 totalSpecular = vec3(0, 0, 0);
    
    for(int i = 0; i < lightCount; ++i)
    {
        vec3 lightPos = lightPositions[i];
        float dsq = distancesq(lightPos, FragPos);
        vec3 I = lightIntensities[i] / dsq;
        vec3 L = normalize(lightPos - FragPos);
        vec3 V = normalize(cameraPos - FragPos);
        vec3 H = normalize(L + V);
        vec3 N = normalize(Normal);
        
        float NdotL = dot(N, L); // for diffuse component
        float NdotH = dot(N, H); // for specular component

        vec3 diffuseColor = I * Diffuse * max(0, NdotL);
        vec3 specularColor = I * Specular * pow(max(0, NdotH), 100);
        
        totalDiffuse += diffuseColor;
        totalSpecular += specularColor;
    }
    
    // vec3 ambientColor = Iamb * ka;
    vec3 ambientColor = vec3(0);

    FragColor = vec4(totalDiffuse + totalSpecular + ambientColor, 1);
}
