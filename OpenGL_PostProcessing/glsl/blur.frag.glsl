#version 330 core

in vec2 fs_UV;
out vec3 color;

uniform sampler2D u_Texture;

float kernel[121] = float[](0.006849, 0.007239, 0.007559, 0.007795, 0.007941, 0.00799, 0.007941, 0.007795, 0.007559, 0.007239, 0.006849,
                            0.007239, 0.007653, 0.00799, 0.00824, 0.008394, 0.008446, 0.008394, 0.00824, 0.00799, 0.007653, 0.007239,
                            0.007559, 0.00799, 0.008342, 0.008604, 0.008764, 0.008819, 0.008764, 0.008604, 0.008342, 0.00799, 0.007559,
                            0.007795, 0.00824, 0.008604, 0.008873, 0.009039, 0.009095, 0.009039, 0.008873, 0.008604, 0.00824, 0.007795,
                            0.007941, 0.008394, 0.008764, 0.009039, 0.009208, 0.009265, 0.009208, 0.009039, 0.008764, 0.008394, 0.007941,
                            0.00799, 0.008446, 0.008819, 0.009095, 0.009265, 0.009322, 0.009265, 0.009095, 0.008819, 0.008446, 0.00799,
                            0.007941, 0.008394, 0.008764, 0.009039, 0.009208, 0.009265, 0.009208, 0.009039, 0.008764, 0.008394, 0.007941,
                            0.007795, 0.00824, 0.008604, 0.008873, 0.009039, 0.009095, 0.009039, 0.008873, 0.008604, 0.00824, 0.007795,
                            0.007559, 0.00799, 0.008342, 0.008604, 0.008764, 0.008819, 0.008764, 0.008604, 0.008342, 0.00799, 0.007559,
                            0.007239, 0.007653, 0.00799, 0.00824, 0.008394, 0.008446, 0.008394, 0.00824, 0.00799, 0.007653, 0.007239,
                            0.006849, 0.007239, 0.007559, 0.007795, 0.007941, 0.00799, 0.007941, 0.007795, 0.007559, 0.007239, 0.006849);

const int radius = 5;
const int dim = 11;

void main()
{
    // TODO: Compute the weighted average of the 11x11 set of pixels
    // in u_Texture surrounding the current fragment's location.
    // The weights are stored in the array above; index into it
    // using the same method you used to index into the Z buffer
    // in homework 3.
    ivec2 texSize = textureSize(u_Texture, 0);
    vec2 onePixel = vec2(1.0) / vec2(texSize);

    vec3 finalColor = vec3(0.0);

    for(int y = -radius; y <= radius; y++) {
        for(int x = -radius; x <= radius; x++) {
            vec2 offset = vec2(float(x), float(y)) * onePixel;
            vec3 texel = texture(u_Texture, fs_UV + offset).rgb;
            float weight = kernel[(y + radius) * dim + (x + radius)];
            finalColor += texel * weight;
        }
    }

    color = finalColor;
}
