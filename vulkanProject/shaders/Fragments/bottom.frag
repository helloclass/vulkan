#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform sampler2D texSampler;
layout(set = 0, binding = 2) uniform sampler2D alphaSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 normalVector;
layout(location = 2) in vec3 lightPosition;
layout(location = 3) in vec3 cameraPosition;
layout(location = 4) in vec3 halfPosition;
layout(location = 5) in vec3 testshadow;
layout(location = 6) in mat3 modelMatrix;

layout(location = 0) out vec4 outColor;

void main() {


    outColor = texture(texSampler, fragTexCoord) /* vec4(testshadow, 1.0)*/;
}