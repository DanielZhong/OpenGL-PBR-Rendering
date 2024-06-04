#version 330 core

in vec3 vs_Pos;
in vec2 vs_UV;

out vec3 fs_Pos;
out vec2 fs_UV;

void main()
{
    fs_Pos = vs_Pos;
    fs_UV = vs_UV;

    gl_Position = vec4(vs_Pos, 1.0);
}
