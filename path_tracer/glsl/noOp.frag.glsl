#version 330 core

uniform sampler2D u_Texture;
uniform vec2 u_ScreenDims;
uniform int u_Iterations;

in vec3 fs_Pos;
in vec2 fs_UV;

out vec4 out_Col;
vec3 ReinhardToneMapping(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 GammaCorrection(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}

void main() {
    vec4 color = texture(u_Texture, fs_UV);

    vec3 toneMappedColor = ReinhardToneMapping(color.rgb);
    vec3 gammaCorrectedColor = GammaCorrection(toneMappedColor, 2.2);

    out_Col = vec4(gammaCorrectedColor, 1.0);
}
