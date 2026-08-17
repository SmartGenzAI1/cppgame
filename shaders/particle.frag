#version 330 core

in vec2 vUV;
uniform vec4 particleColor;

out vec4 FragColor;

void main() {
    vec2 center = vUV - 0.5;
    float dist = length(center);
    if (dist > 0.5) discard;

    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    FragColor = vec4(particleColor.rgb, particleColor.a * alpha);
}
