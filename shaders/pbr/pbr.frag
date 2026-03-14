#version 450

// Inputs from vertex shader
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 3) in mat3 fragTBN;

// Output
layout(location = 0) out vec4 outColor;

// Material texture flags
const uint USE_ALBEDO_TEXTURE         = 1 << 0;
const uint USE_NORMAL_TEXTURE         = 1 << 1;
const uint USE_METALLIC_ROUGHNESS_AO_TEXTURE = 1 << 2;
const uint USE_EMISSIVE_TEXTURE       = 1 << 3;
const uint ALPHA_TEST                 = 1 << 4;

// Scene data (set 0, binding 0)
layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
} scene;

// Material parameters (set 1, binding 0)
layout(set = 1, binding = 0) uniform MaterialParams {
    vec4 albedoColor;
    vec3 emissiveColor;
    float metallic;
    float roughness;
    float ao;
    float alphaCutoff;
    uint flags;
} material;

// Material textures (set 1, bindings 1-4)
layout(set = 1, binding = 1) uniform sampler2D albedoMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessAOMap;
layout(set = 1, binding = 4) uniform sampler2D emissiveMap;

// Light data (simplified - single directional light)
const vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
const vec3 lightColor = vec3(1.0) * 3.0;
const vec3 ambientColor = vec3(0.03, 0.03, 0.03);

// Constants
const float PI = 3.14159265359;

// PBR Functions

// Distribution function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Geometry function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// Geometry function (Smith's method)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel function (Schlick approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Fresnel function with roughness for IBL
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Get material parameters
    vec4 albedo = material.albedoColor;
    float metallic = material.metallic;
    float roughness = material.roughness;
    float ao = material.ao;
    vec3 emissive = material.emissiveColor;

    // Sample textures if enabled
    if ((material.flags & USE_ALBEDO_TEXTURE) != 0u) {
        albedo = texture(albedoMap, fragTexCoord);
    }

    // Alpha test
    if ((material.flags & ALPHA_TEST) != 0u) {
        if (albedo.a < material.alphaCutoff) {
            discard;
        }
    }

    if ((material.flags & USE_METALLIC_ROUGHNESS_AO_TEXTURE) != 0u) {
        vec3 mra = texture(metallicRoughnessAOMap, fragTexCoord).rgb;
        // Metallic in R, Roughness in G, AO in B
        metallic = mra.r;
        roughness = mra.g;
        ao = mra.b;
    }

    if ((material.flags & USE_EMISSIVE_TEXTURE) != 0u) {
        emissive = texture(emissiveMap, fragTexCoord).rgb;
    }

    // Calculate normal (with normal mapping)
    vec3 N = normalize(fragTBN[2]); // Default to vertex normal
    if ((material.flags & USE_NORMAL_TEXTURE) != 0u) {
        vec3 tangentNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
        N = normalize(fragTBN * tangentNormal);
    }

    // Calculate view direction
    vec3 V = normalize(scene.cameraPos - fragWorldPos);

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04); // Base reflectivity for dielectrics
    F0 = mix(F0, albedo.rgb, metallic);

    // Calculate light direction (directional light)
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    // Calculate lighting
    float NdotL = max(dot(N, L), 0.0);

    // Direct lighting
    vec3 Lo = (kD * albedo.rgb / PI + specular) * lightColor * NdotL;

    // Ambient lighting (simple ambient term)
    vec3 ambient = ambientColor * albedo.rgb * ao;

    // Add emissive
    vec3 color = ambient + Lo + emissive;

    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, albedo.a);
}
