#version 410 core

// All of the following variables could be defined in the OpenGL
// program and passed to this shader as uniform variables. This
// would be necessary if their values could change during runtim.
// However, we will not change them and therefore we define them
// here for simplicity.

vec3 Iamb = vec3(0.8, 0.8, 0.8); // ambient light intensity
vec3 kd = vec3(0.2, 0.2, 0.2);     // diffuse reflectance coefficient
vec3 ka = vec3(0.3, 0.3, 0.3);   // ambient reflectance coefficient
vec3 ks = vec3(0.8, 0.8, 0.8);   // specular reflectance coefficient

const int maxLightCount = 100;

uniform vec3 lightPositions[maxLightCount];
uniform vec3 lightIntensities[maxLightCount];
uniform vec3 cameraPos;
uniform int lightCount;

in vec4 fragWorldPos;
in vec3 fragWorldNor;

out vec4 fragColor;

float distancesq(vec3 a, vec3 b){
    vec3 diff = a - b;
    return dot(diff, diff);
}

// Compute lighting. We assume lightPos and eyePos are in world
// coordinates. fragWorldPos and fragWorldNor are the interpolated
// coordinates by the rasterizer.
void main(void)
{
    vec3 totalDiffuse = vec3(0, 0, 0);
    vec3 totalSpecular = vec3(0, 0, 0);
    
    for(int i = 0; i < lightCount; i++){
        vec3 lightPos = lightPositions[i];
        vec3 pos = vec3(fragWorldPos);
        float dsq = distancesq(lightPos, pos);
        vec3 I = lightIntensities[i] / dsq;
        vec3 L = normalize(lightPos - pos);
        vec3 V = normalize(cameraPos - pos);
        vec3 H = normalize(L + V);
        vec3 N = normalize(fragWorldNor);

        float NdotL = dot(N, L); // for diffuse component
        float NdotH = dot(N, H); // for specular component

        vec3 diffuseColor = I * kd * max(0, NdotL);
        vec3 specularColor = I * ks * pow(max(0, NdotH), 100);
        
        totalDiffuse += diffuseColor;
        totalSpecular += specularColor;
    }
    
    // vec3 ambientColor = Iamb * ka;
    vec3 ambientColor = vec3(0);
    
    fragColor = vec4(totalDiffuse + totalSpecular + ambientColor, 1);
}
