#version 330 core

uniform mat4 u_Model;
uniform mat4 u_ModelInvTr;
uniform mat4 u_ViewProj;


in vec3 vs_Pos;
in vec3 vs_Nor;
in vec3 vs_Col;

out vec3 fs_Pos;
out vec3 fs_Nor;

void main()
{
    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = normalize(invTranspose * vs_Nor);

    vec4 modelposition = u_Model * vec4(vs_Pos, 1.f);
    fs_Pos = modelposition.xyz;

    gl_Position = u_ViewProj * modelposition;
}
