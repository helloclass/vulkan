#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 normalVector;
layout(location = 2) in vec3 lightPosition;
layout(location = 3) in vec3 cameraPosition;
layout(location = 4) in vec3 halfPosition;
layout(location = 5) in mat3 modelMatrix;

layout(location = 0) out vec4 outColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}; 

void main() {
    vec3 lightColor = vec3(1.5f);

    Material material;
    // 주변광
    float N = abs(dot(cameraPosition, normalVector));
    material.ambient = (N > 0.00003f) ? vec3 ( 0.3f ) :vec3 ( N * 1000.0f + 0.6f );

    // 산광
    material.diffuse    = vec3 ( 0.0f, 0.0f, 0.4f );
    // 반사광
    material.specular   = vec3 ( 0.8f );
    // 반사광 집중도
    material.shininess  = ( 32.0f );

    // Ambient
    vec3 ambient = vec3(1.0) * lightColor * material.ambient;

    // Diffuse
    vec3 norm = normalize(normalVector);

    vec3 lightDir = normalize(lightPosition - modelMatrix * halfPosition);

    float diff = max(dot(norm, lightDir), 0.3f);
    vec3 diffuse = vec3(1.0) * lightColor * (diff * material.diffuse );

    // Specular
    // 방향성과 크기 또한 고려되는 법선 벡터
    norm = transpose(inverse(modelMatrix)) * normalVector;
    vec3 viewDir = normalize(cameraPosition - modelMatrix * halfPosition);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = vec3(1.0) * spec * lightColor * material.specular;

    outColor = texture(texSampler, fragTexCoord) * vec4(ambient + diffuse + specular, 1.0f);
}