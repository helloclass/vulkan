#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    
    mat4 pitch;
    mat4 yaw;
    mat4 roll;

    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 3) uniform TexelBufferObject {
    vec4 position;
    vec4 color;
} tbo;

layout( push_constant ) uniform TargetVec {
    vec3 CamPos;
    vec3 LightPos;
    vec3 Normal;
} targetVec;

// Normal
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 normalVector;
layout(location = 2) out vec3 lightPosition;
layout(location = 3) out vec3 cameraPosition;
layout(location = 4) out vec3 halfPosition;
layout(location = 5) out vec3 testshadow;
// Located 6, 7, 8
layout(location = 6) out mat3 modelMatrix;
layout(location = 9) out float attenuation;

void main() {
    lightPosition = mat3(   ( ubo.pitch * ubo.yaw * ubo.roll ) *
                            ubo.model) * 
                            inPosition - targetVec.LightPos;
    vec3 lightPos = vec3(-targetVec.LightPos.x, targetVec.LightPos.y, targetVec.LightPos.z);

    float distance = length ( lightPosition );

    attenuation =       1.0 / ( 1.0f + 0.09f * distance + 
                                0.032f * (distance * distance));
    
    vec4 GL_POSITION =  ubo.proj *
                        ubo.view * 
                        ( ubo.pitch * ubo.yaw * ubo.roll ) *
                        ubo.model * 
                        vec4(lightPos, 1.0) * 
                        vec4(inPosition, 1.0);

    vec4 shadow_coords = GL_POSITION / GL_POSITION.w;

    testshadow = vec3(  ((  (shadow_coords.x < shadow_coords.z - 0.005) || 
                            (shadow_coords.y < shadow_coords.z - 0.005) 
                        ) ? 0.2f : 1.0f));
    // testshadow = vec3 (shadow_coords);

    gl_Position =   ubo.proj *
                    ( ubo.pitch * ubo.yaw * ubo.roll ) *  
                    ubo.view *
                    ubo.model * 
                    vec4(inPosition, 1.0);

    fragTexCoord = inTexCoord;
    normalVector = inNormal;
    
    cameraPosition = targetVec.CamPos;
    halfPosition = inPosition;
    modelMatrix = mat3(ubo.model);
}