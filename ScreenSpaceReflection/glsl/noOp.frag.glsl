#version 330 core

uniform sampler2D u_Texture;

in vec2 fs_UV;
out vec4 out_Col;

void main() {
    out_Col = texelFetch(u_Texture, ivec2(gl_FragCoord.xy), 0);
}
