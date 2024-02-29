#version 330 core

uniform mat4 u_View;
uniform mat4 u_Proj;

in vec3 vs_Pos;
in vec3 vs_Nor;

out vec2 fs_UV;

void main()
{
    fs_UV = (normalize(vs_Nor).xy + vec2(1.0)) / 2;

    gl_Position = u_Proj * u_View * vec4(vs_Pos, 1.);
}
