#version 330 core

uniform sampler2D u_TextureSSR;
uniform sampler2D u_Kernel;
uniform int u_KernelRadius;

in vec2 fs_UV;
layout (location = 0) out vec4 out_Col;

void main() {
    // TODO: Apply a Gaussian blur to the screen-space reflection
    // texture using the kernel stored in u_Kernel.

    vec4 color = vec4(0.0);
    float total = 0.0;

    for (int i = -u_KernelRadius; i <= u_KernelRadius; ++i) {
        for (int j = -u_KernelRadius; j <= u_KernelRadius; ++j) {
            vec2 offset = vec2(i, j) / textureSize(u_TextureSSR, 0);
            float weight = texture(u_Kernel, vec2(0.5) + vec2(i, j) / float(u_KernelRadius * 2 + 1)).r;
            color += texture(u_TextureSSR, fs_UV + offset) * weight;
            total += weight;
        }
    }

    out_Col = color / total;
}
