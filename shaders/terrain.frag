#version 330 core

in vec3 FragWorldPos;
in vec3 FragNormal;
in vec2 FragUV;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 fogColor;
uniform float fogDensity;
uniform vec3 cameraPos;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(FragNormal);
    float slope = 1.0 - normal.y;

    vec3 deepWater  = vec3(0.05, 0.15, 0.4);
    vec3 waterColor = vec3(0.1, 0.4, 0.65);
    vec3 sandColor  = vec3(0.76, 0.70, 0.50);
    vec3 grassColor = vec3(0.18, 0.52, 0.12);
    vec3 darkGrass  = vec3(0.12, 0.38, 0.08);
    vec3 rockColor  = vec3(0.45, 0.42, 0.38);
    vec3 snowColor  = vec3(0.95, 0.95, 0.97);

    float h = FragWorldPos.y;
    float slopeBlend = smoothstep(0.15, 0.45, slope);

    vec3 color;
    if (h < -0.5) {
        color = deepWater;
    } else if (h < 0.2) {
        float t = smoothstep(-0.5, 0.2, h);
        color = mix(deepWater, waterColor, t);
    } else if (h < 1.0) {
        float t = smoothstep(0.2, 1.0, h);
        color = mix(waterColor, sandColor, t);
    } else if (h < 4.0) {
        float t = smoothstep(1.0, 4.0, h);
        vec3 low = mix(sandColor, grassColor, smoothstep(1.0, 2.0, h));
        color = mix(low, darkGrass, t);
        color = mix(color, rockColor, slopeBlend);
    } else if (h < 10.0) {
        float t = smoothstep(4.0, 10.0, h);
        color = mix(darkGrass, rockColor, t);
        color = mix(color, rockColor, slopeBlend * 0.7);
    } else {
        float t = smoothstep(10.0, 14.0, h);
        color = mix(rockColor, snowColor, t);
    }

    float diff = max(dot(normal, sunDirection), 0.0);
    vec3 diffuse = diff * sunColor;
    vec3 ambient = 0.28 * vec3(1.0);

    vec3 viewDir = normalize(cameraPos - FragWorldPos);
    vec3 halfway = normalize(sunDirection + viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), 32.0);
    vec3 specular = spec * sunColor * 0.15;

    vec3 result = color * (ambient + diffuse) + specular;

    float dist = length(cameraPos - FragWorldPos);
    float fogFactor = exp(-fogDensity * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, 1.0);
}
