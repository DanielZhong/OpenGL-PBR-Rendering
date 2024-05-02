#version 330 core
in vec3 vs_Pos;

uniform mat4 u_ViewProj;

out vec3 fs_Pos;

void main() {
    fs_Pos = vs_Pos;
    vec4 clipPos = u_ViewProj * vec4(vs_Pos, 1.0);

    gl_Position = clipPos.xyww;
}
