#version 330 core

layout(location = 0) in vec2 aPos;

uniform mat4 invViewProjection;

out vec3 WorldDirection;

void main() {
    gl_Position = vec4(aPos, 0.9999, 1.0);

    vec4 clipPos = vec4(aPos, 1.0, 1.0);
    vec4 worldPos = invViewProjection * clipPos;
    WorldDirection = worldPos.xyz / worldPos.w;
}
