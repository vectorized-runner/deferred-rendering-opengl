#version 410 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout(location=0) in vec3 inVertex;
layout(location=1) in vec3 inNormal;

out vec4 fragWorldPos;
out vec3 fragWorldNor;

void main(void)
{
    // Compute the world coordinates of the vertex and its normal.
    // These coordinates will be interpolated during the rasterization
    // stage and the fragment shader will receive the interpolated
    // coordinates.

    fragWorldPos = model * vec4(inVertex, 1);
    fragWorldNor = inverse(transpose(mat3x3(model))) * inNormal;

    gl_Position = projection * view * model * vec4(inVertex, 1);
}

