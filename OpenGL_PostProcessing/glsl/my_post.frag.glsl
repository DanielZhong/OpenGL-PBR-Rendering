#version 330 core

in vec2 fs_UV;
out vec3 color;

uniform sampler2D u_Texture;
uniform float u_Time;

void main() {
    vec2 uv = fs_UV;

    // Warping effect
    vec2 warpedUv = uv;
    float line = mod(u_Time * 0.001, 1.0);
    float offset = line - uv.y;
    if (abs(offset) < 0.1) {
        offset += 0.1;
        int bdoffset = int(offset * 500.0) / 4;
        warpedUv.x += 0.03 * (tan(float(bdoffset) / 1.25) + tan(offset * 2.0));
        warpedUv = vec2(mod(warpedUv.x, 1.0), mod(warpedUv.y, 1.0));
    }
    vec2 redUV = fs_UV;
    vec2 greenUV = fs_UV;
    vec2 blueUV = fs_UV;

    float redOffsetAmount = -0.01;
    float greenOffsetAmountX = 0.1;
    float greenOffsetAmountY = 0.1;

    if (sin(u_Time * 0.1) >= 0.0) {
        redOffsetAmount = -0.01;
        greenOffsetAmountX = 0.01;
        greenOffsetAmountY = 0.01;
    }

    redUV = warpedUv - vec2(redOffsetAmount, 0.0);
    greenUV = warpedUv + vec2(greenOffsetAmountX, greenOffsetAmountY);

    blueUV = warpedUv;

    float red = texture(u_Texture, redUV).r;
    float green = texture(u_Texture, greenUV).g;
    float blue = texture(u_Texture, blueUV).b;
    color = vec3(red, green, blue);
}
