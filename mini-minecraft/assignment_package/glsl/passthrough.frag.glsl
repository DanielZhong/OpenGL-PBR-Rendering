#version 330 core

in vec2 fs_UV;

uniform sampler2D u_Albedo;
uniform sampler2D u_NormalTexture;
uniform float u_Time; // Time uniform for dynamic effects

out vec4 fragColor;

vec3 applyColorGrading(vec3 color) {
    // Simple color grading: increase contrast and adjust colors
    color = pow(color, vec3(1.2)); // Increase contrast
    color = mix(color, vec3(1.0, 0.95, 0.8), 0.1); // Warm tint
    return color;
}

void main() {
    vec4 albedo = texture(u_Albedo, fs_UV);
    vec3 gradedColor = applyColorGrading(albedo.rgb);
    fragColor = vec4(gradedColor, albedo.a);
}
