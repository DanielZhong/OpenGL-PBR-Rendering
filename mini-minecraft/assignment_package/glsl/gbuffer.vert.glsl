#version 330
// ^ Change this to version 130 if you have compatibility issues

//This is a vertex shader. While it is called a "shader" due to outdated conventions, this file
//is used to apply matrix transformations to the arrays of vertex data passed to it.
//Since this code is run on your GPU, each vertex is transformed simultaneously.
//If it were run on your CPU, each vertex would have to be processed in a FOR loop, one at a time.
//This simultaneous transformation allows your program to run much faster, especially when rendering
//geometry with millions of vertices.

uniform mat4 u_Model;       // The matrix that defines the transformation of the
                            // object we're rendering. In this assignment,
                            // this will be the result of traversing your scene graph.

uniform mat4 u_ModelInvTr;  // The inverse transpose of the model matrix.
                            // This allows us to transform the object's normals properly
                            // if the object has been non-uniformly scaled.

uniform mat4 u_ViewProj;    // The matrix that defines the camera's transformation.
                            // We've written a static matrix for you to use for HW2,
                            // but in HW3 you'll have to generate one yourself

uniform vec4 u_Color;       // When drawing the cube instance, we'll set our uniform color to represent different block types.

uniform float u_Time;

uniform sampler2D u_NormalTexture;

in vec4 vs_Pos;             // The array of vertex positions passed to the shader

in vec4 vs_Nor;             // The array of vertex normals passed to the shader

in vec4 vs_Col;             // The array of vertex colors passed to the shader.

in vec2 vs_UV;

in vec2 vs_Animated;

in vec3 vs_Tangent;

in vec3 vs_Bitangent;

out vec4 fs_Pos;
out vec4 fs_Nor;            // The array of normals that has been transformed by u_ModelInvTr. This is implicitly passed to the fragment shader.
out vec4 fs_Col;            // The color of each vertex. This is implicitly passed to the fragment shader.
out vec2 fs_UV;
out vec2 fs_Animated;
out vec3 fs_Tan;
out vec3 fs_Bit;
out vec4 shadow_coord;
out vec4 fs_WorldPos;
uniform mat4 u_DepthMVP;

const vec4 lightDir = normalize(vec4(0.5, 1, 0.75, 0));  // The direction of our virtual light, which is used to compute the shading of
                                        // the geometry in the fragment shader.
// Constants and sea parameters
const float SEA_FREQ = 0.16;
const float SEA_HEIGHT = 0.6;
const float SEA_CHOPPY = 4.0;
const mat2 octave_m = mat2(1.6, 1.2, -1.2, 1.6);

// Noise function to simulate water surface dynamics
float hash(vec2 p) {
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(h) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return -1.0 + 2.0 * mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
                            mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

// Function to calculate sea height at a given position
float sea_height(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    vec2 uv = p.xz * freq;
    float h = 0.0;
    for (int i = 0; i < 3; i++) {
        h += noise(uv + u_Time * 0.005 * SEA_CHOPPY) * amp;
        uv *= octave_m;
        amp *= 0.8;
    }
    return h;
}

void main() {
    vec4 modelPosition = u_Model * vs_Pos;
    vec4 clipPosition = u_ViewProj * modelPosition;

    // Apply sea wave effects for water vertices
    if (vs_Animated.x == 1.0f) {
        float heightOffset = sea_height(modelPosition.xyz);
        clipPosition.y += -0.8 + heightOffset; // Adjust y-coordinate based on wave height
    }

    // Pass transformed positions and normals
    fs_Pos = modelPosition;
    fs_Col = vs_Col;
    fs_UV = (vs_Animated.x > 0.f) ? vec2(vs_UV.x + float(mod(u_Time, 100.f) / 100.f) * 0.0625f, vs_UV.y) : vs_UV;
    fs_Animated = vs_Animated;
    mat3 normalMatrix = mat3(u_ModelInvTr);
    fs_Nor = vec4(normalMatrix * vec3(vs_Nor), 0.0);
    fs_Tan = normalize(mat3(u_Model) * vs_Tangent);
    fs_Bit = normalize(mat3(u_Model) * vs_Bitangent);

    shadow_coord = u_DepthMVP * modelPosition;
    fs_WorldPos = modelPosition;
    gl_Position = clipPosition;
}
