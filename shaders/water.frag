#version 330 core

in vec3 FragWorldPos;
in vec3 FragNormal;
in vec2 FragUV;

uniform vec3 cameraPos;
uniform float time;
uniform vec3 sunDirection;

out vec4 FragColor;

void main() {
    vec2 uv = FragUV + vec2(time * 0.02, time * 0.01);
    float wave = sin(uv.x * 12.0 + time * 2.0) * 0.02 +
                 sin(uv.y * 8.0 + time * 1.5) * 0.02;

    vec3 waterColor = mix(
        vec3(0.05, 0.20, 0.45),
        vec3(0.1, 0.45, 0.7),
        0.5 + wave
    );

    vec3 viewDir = normalize(cameraPos - FragWorldPos);
    vec3 normal = normalize(FragNormal + vec3(wave, 0.0, wave * 0.5));

    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 3.0);
    vec3 skyReflect = vec3(0.53, 0.81, 0.92);

    float diff = max(dot(normal, sunDirection), 0.0);
    vec3 specular = pow(max(dot(reflect(-sunDirection, normal), viewDir), 0.0), 64.0) * vec3(1.0);

    vec3 result = mix(waterColor * (0.3 + 0.7 * diff), skyReflect, fresnel * 0.6) + specular * 0.3;

    FragColor = vec4(result, 0.8);
}
