#version 410 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform vec3 tint;
uniform samplerCube cubemap;

void main()
{
    vec3 I = normalize(Position - cameraPos);
    vec3 R = reflect(I, normalize(Normal));
    FragColor = vec4(texture(cubemap, R).rgb * 0.5, 1.0) + vec4(tint, 1.0);
}
