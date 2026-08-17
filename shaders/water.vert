#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 view;
uniform mat4 projection;
uniform float waterLevel;

out vec3 FragWorldPos;
out vec3 FragNormal;
out vec2 FragUV;

void main() {
    FragWorldPos = aPos;
    FragNormal = aNormal;
    FragUV = aPos.xz * 0.02;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
