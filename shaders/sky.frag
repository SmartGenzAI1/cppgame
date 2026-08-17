#version 330 core

in vec3 WorldDirection;

uniform vec3 sunDirection;

out vec4 FragColor;

void main() {
    vec3 dir = normalize(WorldDirection);

    float y = dir.y;

    vec3 topColor = vec3(0.05, 0.1, 0.35);
    vec3 horizonColor = vec3(0.5, 0.6, 0.8);
    vec3 sunHorizonColor = vec3(0.9, 0.5, 0.2);

    float horizonFactor = pow(1.0 - max(y, 0.0), 3.0);
    vec3 skyColor = mix(topColor, horizonColor, horizonFactor);

    float sunHorizonFactor = pow(1.0 - max(y, 0.0), 8.0);
    skyColor = mix(skyColor, sunHorizonColor, sunHorizonFactor * 0.5);

    vec3 sunDir = normalize(sunDirection);
    float sunDot = dot(dir, sunDir);
    float sunDisk = smoothstep(0.999, 0.9999, sunDot);
    float sunGlow = pow(max(sunDot, 0.0), 256.0);

    vec3 sunDiskColor = vec3(1.0, 0.95, 0.8);
    skyColor += sunDiskColor * sunDisk * 2.0;
    skyColor += sunDiskColor * sunGlow * 0.3;

    FragColor = vec4(skyColor, 1.0);
}
