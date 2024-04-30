#version 330 core

in vec4 vs_Pos;
in vec4 vs_Col;
in vec2 vs_UV;

uniform sampler2D u_NormalTexture;

out vec4 fs_Pos;
out vec4 fs_Col;
out vec2 fs_UV;

void main()
{
    fs_Col = vs_Col;
    fs_UV = vs_UV; /*+ vec2(0.2, 0.2)*/
    fs_Pos = vs_Pos;
    gl_Position = vs_Pos;

}
