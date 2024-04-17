#version 330 core

// Compute the irradiance across the entire
// hemisphere aligned with a surface normal
// pointing in the direction of fs_Pos.
// Thus, our surface normal direction
// is normalize(fs_Pos).

in vec3 fs_Pos;
out vec4 out_Col;
uniform samplerCube u_EnvironmentMap;

const float PI = 3.14159265359;

void main()
{
    vec3 N = normalize(fs_Pos); // Surface normal
    vec3 irradiance = vec3(0.0);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right)); // we are going to sample from tangent space

    float sampleDelta = 0.025; // step size of sampling direction within the hemisphere aligned with normal N. Smaller step size means more samples
    float nrSamples = 0.0;

    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            // Spherical to Cartesian coordinates (tangent space)
            vec3 wi = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            // Tangent space to world
            vec3 wiW = wi.x * right + wi.y * up + wi.z * N;

            irradiance += texture(u_EnvironmentMap, wiW).rgb * cos(theta) * sin(theta);
            ++nrSamples;
        }
    }

    irradiance = PI * irradiance / nrSamples;

    out_Col = vec4(irradiance, 1.0);
}
