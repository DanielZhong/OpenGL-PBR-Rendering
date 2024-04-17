#version 330 core

uniform mat4 u_Model;
uniform mat4 u_ModelInvTr;
uniform mat4 u_ViewProj;


uniform sampler2D u_NormalMap;
uniform bool u_UseNormalMap;
uniform sampler2D u_DisplacementMap;
uniform float u_DisplacementMagnitude;
uniform bool u_UseDisplacementMap;


in vec3 vs_Pos;
in vec3 vs_Nor; // Surface normal
in vec3 vs_Tan; // Surface tangent
in vec3 vs_Bit; // Surface bitangent
in vec2 vs_UV;

out vec3 fs_Pos;
out vec3 fs_Nor;
out vec3 fs_Tan;
out vec3 fs_Bit;
out vec2 fs_UV;


void main()
{
    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = normalize(invTranspose * vs_Nor);
    fs_Tan = normalize(mat3(u_Model) * vs_Tan);
    fs_Bit = normalize(mat3(u_Model) * vs_Bit);

    vec3 N = vs_Nor;
    vec3 displacedPos = vs_Pos;
    if(u_UseDisplacementMap) {
        float displacement = texture(u_DisplacementMap, vs_UV).r;
        displacedPos += vs_Nor * displacement * u_DisplacementMagnitude;
    }

    vec4 modelposition = u_Model * vec4(displacedPos, 1.f);
    fs_Pos = modelposition.xyz;

    fs_UV = vs_UV;

    gl_Position = u_ViewProj * modelposition;
}
