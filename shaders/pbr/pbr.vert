#version 450

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

// Outputs to fragment shader
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;

// Scene data (set 0, binding 0)
layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
} scene;

// Model matrix (push constant)
layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {
    // Calculate world position
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    // Pass texture coordinates
    fragTexCoord = inTexCoord;

    // Calculate world normal and tangent
    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));
    fragNormal = normalize(normalMatrix * inNormal);
    fragTangent = normalize(normalMatrix * inTangent);

    // Calculate clip space position
    gl_Position = scene.projection * scene.view * worldPos;
}
