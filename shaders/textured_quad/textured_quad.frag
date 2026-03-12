#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // Use vertex colors and create a simple pattern based on texture coordinates
    vec3 color = fragColor;

    // Add a subtle grid pattern
    float grid = step(0.95, fract(fragTexCoord.x * 10.0)) + step(0.95, fract(fragTexCoord.y * 10.0));
    color = mix(color, vec3(1.0), grid * 0.3);

    outColor = vec4(color, 1.0);
}
