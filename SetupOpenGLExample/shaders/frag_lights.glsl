#version 410 core

uniform vec3 unlit;

out vec4 fragColor;

void main(void)
{
    fragColor = vec4(unlit, 1);
}
