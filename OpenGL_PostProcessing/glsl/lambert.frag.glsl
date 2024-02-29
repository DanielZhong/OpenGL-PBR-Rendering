#version 330 core

uniform sampler2D u_Texture; // An object that lets us access
                             // the texture file we project onto
                             // our model's surface
uniform vec3 u_CamLook; // the camera's forward vector

in vec3 fs_Nor;
in vec2 fs_UV;

out vec4 out_Color;

void main()
{
    // Obtaining the material's base color from
    // the texture file
    vec3 albedo = texture(u_Texture, fs_UV).rgb;
    // Lambertian term computed using the camera's
    // forward vector as the light direction
    albedo *= dot(-u_CamLook, fs_Nor);
    out_Color = vec4(albedo, 1.);
}
