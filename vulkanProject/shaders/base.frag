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
    vec3 lightColor = vec3( 1.0f );

    Material material;
    // 주변광
    material.ambient    = vec3 ( 1.3f );
    // 산광
    material.diffuse    = vec3 ( 0.0f, 0.0f, 0.4f );
    // 반사광
    material.specular   = vec3 ( 0.8f );
    // 반사광 집중도
    material.shininess  = ( 32.0f );

    // Ambient
    vec3 ambient = vec3(1.0) * lightColor * material.ambient;

    outColor = texture(texSampler, fragTexCoord) * vec4(ambient, 1.0f);
}