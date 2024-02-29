#version 330

uniform sampler2D u_Texture; // The texture to be read from by this shader
uniform float u_Time;
uniform vec3 u_CamLook;

in vec3 fs_Pos;
in vec3 fs_Nor;
in vec2 fs_UV;

layout(location = 0) out vec3 out_Color;

void main()
{
    vec3 normal = normalize(fs_Nor);
    float lambert = dot(normal, -u_CamLook);

    float levels = 4.0;
    float toon = floor(lambert * levels) / levels;

    vec3 albedo = texture(u_Texture, fs_UV).rgb;

    vec3 dynamicColor = vec3(
                0.75 * sin(u_Time * 0.05) + 0.25,
                0.75 * cos(u_Time * 0.05) + 0.25,
                0.75 * -cos(u_Time * 0.05) + 0.25
                );

    float blendFactor = (sin(u_Time * 0.1) + 1.0) * 0.5;
    vec3 blendedColor = mix(albedo, dynamicColor, blendFactor);

    out_Color = blendedColor * toon;
}
