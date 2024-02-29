#version 330 core

uniform ivec2 u_Dimensions;
uniform float u_Time;

in vec2 fs_UV;

out vec3 color;

uniform sampler2D u_Texture;

vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

// cited: https://github.com/phaserjs/phaser-ce-examples/blob/master/examples/assets/shaders/bacteria.frag
float worley(vec2 p) {
    float d = 1e30; // Start with a very large distance
    for (int xo = -1; xo <= 1; ++xo) {
        for (int yo = -1; yo <= 1; ++yo) {
            vec2 tp = floor(p) + vec2(xo, yo); // represents the bottom-left corner of the grid cell in which p is located
            d = min(d, length(p - tp - random2(tp))); // Calculate distance to random point in the grid cell
        }
    }
    return d; // The shortest distance to a feature point
}

float pointillism(vec2 uv, float scale) {
    uv *= u_Dimensions.xy;
    // while a smaller scale value will compress the UV coordinates, resulting in more, smaller cells
    float worleyValue = worley(uv / scale);
    worleyValue = 1.0 - smoothstep(0.0, 0.5 / scale, worleyValue);
    return worleyValue;
}

void main() {
    float scale = 0.8;

    // convert grey scale, rgb, human eye more sensitive to green
    // Darker color result greater grey scale
    float gray = dot(texelFetch(u_Texture, ivec2(gl_FragCoord.xy), 0).rgb, vec3(0.299, 0.587, 0.114));

    float radius = pointillism(fs_UV, scale);

    if (sin(radius * u_Time * 0.01) > gray) {
        color = vec3(0.0);
    } else {
        color = vec3(1.0);
    }
}
