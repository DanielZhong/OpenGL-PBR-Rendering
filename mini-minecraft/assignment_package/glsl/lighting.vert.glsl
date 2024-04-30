#version 330 core

in vec4 vs_Pos;
in vec4 vs_Col;
in vec2 vs_UV;

out vec4 fs_Pos;
out vec4 fs_Col;
out vec2 fs_UV;

void main()
{
    fs_Col      = vs_Col;
    fs_UV       = vs_UV;
    fs_Pos      = vs_Pos;
    gl_Position = vs_Pos;
}
