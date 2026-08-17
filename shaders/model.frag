#version 330 core

in vec3 FragWorldPos;
in vec3 FragNormal;

uniform vec3 objectColor;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 cameraPos;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float emissive;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(FragNormal);
    vec3 light = normalize(lightDir);

    float diff = max(dot(normal, light), 0.0);

    vec3 viewDir = normalize(cameraPos - FragWorldPos);
    vec3 halfwayDir = normalize(light + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);

    vec3 ambient = 0.2 * lightColor;
    vec3 diffuse = diff * lightColor;
    vec3 specular = spec * lightColor * 0.4;

    vec3 result = objectColor * (ambient + diffuse) + specular;
    result += objectColor * emissive;

    float dist = length(cameraPos - FragWorldPos);
    float fogFactor = exp(-fogDensity * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, 1.0);
}
