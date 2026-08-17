#version 330 core

layout(location = 0) in vec3 aPos;

uniform vec3 particleCenter;
uniform float particleSize;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

out vec2 vUV;

void main() {
    vec3 worldPos = particleCenter
        + cameraRight * aPos.x * particleSize
        + cameraUp * aPos.y * particleSize;
    vUV = aPos.xy + 0.5;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
