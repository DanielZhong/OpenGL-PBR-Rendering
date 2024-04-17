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
in vec3 vs_Nor;
in vec3 vs_Tan;
in vec3 vs_Bit;
in vec2 vs_UV;

out vec3 fs_Pos;
out vec3 fs_Nor;
out vec3 fs_Tan;
out vec3 fs_Bit;
out vec2 fs_UV;


void coordinateSystem(in vec3 nor, out vec3 tan, out vec3 bit) {
    if (abs(nor.x) > abs(nor.y))
        tan = vec3(-nor.z, 0, nor.x) / sqrt(nor.x * nor.x + nor.z * nor.z);
    else
        tan = vec3(0, nor.z, -nor.y) / sqrt(nor.y * nor.y + nor.z * nor.z);
    bit = cross(nor, tan);
}

void main()
{
    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = normalize(invTranspose * vs_Nor);
    fs_Tan = normalize(mat3(u_Model) * vs_Tan);
    fs_Bit = normalize(mat3(u_Model) * vs_Bit);

    vec3 N = vs_Nor;
//    if(u_UseNormalMap) {
//        vec3 mapNor = texture(u_NormalMap, fs_UV).rgb * 2.0 - vec3(1.0);
//        vec3 tan, bit;
//        coordinateSystem(N, tan, bit);
//        N = mat3(tan, bit, N) * mapNor;
//    }
    vec3 displacedPos = vs_Pos;
    if(u_UseDisplacementMap) {
        float t = texture(u_DisplacementMap, vs_UV).r;
        displacedPos = N * u_DisplacementMagnitude * t + displacedPos;
    }

    vec4 modelposition = u_Model * vec4(displacedPos, 1.f);
    fs_Pos = modelposition.xyz;

    fs_UV = vs_UV;

    gl_Position = u_ViewProj * modelposition;
}
