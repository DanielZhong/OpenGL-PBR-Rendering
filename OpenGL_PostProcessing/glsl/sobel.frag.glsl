#version 330 core

in vec2 fs_UV;
out vec3 color;

uniform sampler2D u_Texture;


void main() {
    // TODO: Use texelFetch to sample u_Texture
    // at each of the nine pixels at and surrounding
    // this shader's fragment. Multiply each sample by the
    // associated cell in Gx, and add it to a sum of horizontal
    // color deltas. Do the same for Gy and a sum of vertical
    // color deltas. Then, output the square root of the sum
    // of both deltas squared.

    ivec2 texSize = textureSize(u_Texture, 0);
    ivec2 pixelCoords = ivec2(fs_UV * vec2(texSize));
    mat3 Gx = mat3( 3, 0, -3,
                    10, 0, -10,
                    3, 0, -3 );

    mat3 Gy = mat3( 3, 10, 3,
                    0,  0, 0,
                    -3, -10, -3 );

    vec3 sumX = vec3(0.0);
    vec3 sumY = vec3(0.0);

    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            ivec2 offset = ivec2(i, j);
            vec3 texel = texelFetch(u_Texture, pixelCoords + offset, 0).rgb;

            sumX += texel * Gx[i+1][j+1];
            sumY += texel * Gy[i+1][j+1];
        }
    }
    // if no edge/same the gradient magnitude will be low (sobel kernel negate cancel each other out)
    vec3 gradient = sqrt(sumX * sumX + sumY * sumY);
    color = gradient;
}
