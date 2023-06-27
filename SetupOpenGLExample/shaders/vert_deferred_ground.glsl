#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1);
    TexCoord = aTexCoord;
    
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    
    // We know that normal is (0, 1, 0 for ground, very hardcoded)
    // mat3 normalMatrix = transpose(inverse(mat3(model)));
    // Normal = normalMatrix * aNormal;
    Normal = vec3(0, 1, 0);
}

