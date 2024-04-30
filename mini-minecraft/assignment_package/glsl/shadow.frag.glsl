#version 330 core
uniform bool u_Transparent;

layout(location = 0) out vec4 fragmentDepth; // Use fragmentDepth as color output

void main(){
    // Output the depth value explicitly to a color buffer
    fragmentDepth = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}
