#version 330 core

uniform mat4 u_MVP; // Model-View-Projection matrix

in vec3 vs_Pos; // Vertex position
in vec3 vs_Nor; // Vertex normal
in vec2 vs_UV; // Vertex texture coordinate

out vec3 fs_Nor; // Surface normal passed down to the fragment shader
out vec2 fs_UV; // Texture coordinate passed down to the fragment shader

void main()
{
    fs_UV = vs_UV;
    fs_Nor = vs_Nor;
    gl_Position = u_MVP * vec4(vs_Pos, 1.);
}
