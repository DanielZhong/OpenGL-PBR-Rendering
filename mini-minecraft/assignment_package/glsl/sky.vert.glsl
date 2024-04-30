#version 330 core

in vec4 vs_Pos;
in vec4 vs_Col;
in vec2 vs_UV;

void main()
{
    gl_Position = vs_Pos;
}
