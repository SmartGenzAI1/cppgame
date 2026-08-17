#version 330 core

in vec2 vUV;
uniform vec4 color;
uniform int useTexture;
uniform sampler2D tex;

out vec4 FragColor;

void main() {
    if (useTexture == 1) {
        vec4 sampled = texture(tex, vUV);
        FragColor = vec4(color.rgb, color.a * sampled.a);
    } else {
        FragColor = color;
    }
}
