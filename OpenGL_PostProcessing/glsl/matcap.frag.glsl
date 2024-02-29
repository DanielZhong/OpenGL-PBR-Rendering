#version 330 core

uniform sampler2D u_Texture;

in vec2 fs_UV;

layout(location = 0) out vec3 out_Col;

void main()
{
    out_Col = vec3(texture(u_Texture, fs_UV));
}
