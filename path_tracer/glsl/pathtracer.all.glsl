#version 330 core

// Uniforms
uniform vec3 u_Eye;
uniform vec3 u_Forward;
uniform vec3 u_Right;
uniform vec3 u_Up;

uniform int u_Iterations;
uniform vec2 u_ScreenDims;

uniform sampler2D u_AccumImg;
uniform sampler2D u_EnvironmentMap;

// Varyings
in vec3 fs_Pos;
in vec2 fs_UV;
out vec4 out_Col;

// Numeric constants
#define PI               3.14159265358979323
#define TWO_PI           6.28318530717958648
#define FOUR_PI          12.5663706143591729
#define INV_PI           0.31830988618379067
#define INV_TWO_PI       0.15915494309
#define INV_FOUR_PI      0.07957747154594767
#define PI_OVER_TWO      1.57079632679489662
#define ONE_THIRD        0.33333333333333333
#define E                2.71828182845904524
#define INFINITY         1000000.0
#define OneMinusEpsilon  0.99999994
#define RayEpsilon       0.000005

// Path tracer recursion limit
#define MAX_DEPTH 10

// Area light shape types
#define RECTANGLE 1
#define SPHERE 2

// Material types
#define DIFFUSE_REFL    1
#define SPEC_REFL       2
#define SPEC_TRANS      3
#define SPEC_GLASS      4
#define MICROFACET_REFL 5
#define PLASTIC         6
#define DIFFUSE_TRANS   7

// Data structures
struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Material {
    vec3  albedo;
    float roughness;
    float eta; // For transmissive materials
    int   type; // Refer to the #defines above

    // Indices into an array of sampler2Ds that
    // refer to a texture map and/or roughness map.
    // -1 if they aren't used.
    int   albedoTex;
    int   normalTex;
    int   roughnessTex;
};

struct Intersection {
    float t;
    vec3  nor;
    vec2  uv;
    vec3  Le; // Emitted light
    int   obj_ID;
    Material material;
};

struct Transform {
    mat4 T;
    mat4 invT;
    mat3 invTransT;
    vec3 scale;
};

struct AreaLight {
    vec3 Le;
    int ID;

    // RECTANGLE, BOX, SPHERE, or DISC
    // They are all assumed to be "unit size"
    // and are altered from that size by their Transform
    int shapeType;
    Transform transform;
};

struct PointLight {
    vec3 Le;
    int ID;
    vec3 pos;
};

struct SpotLight {
    vec3 Le;
    int ID;
    float innerAngle, outerAngle;
    Transform transform;
};

struct Sphere {
    vec3 pos;
    float radius;

    Transform transform;
    int ID;
    Material material;
};

struct Rectangle {
    vec3 pos;
    vec3 nor;
    vec2 halfSideLengths; // Dist from center to horizontal/vertical edge

    Transform transform;
    int ID;
    Material material;
};

struct Box {
    vec3 minCorner;
    vec3 maxCorner;

    Transform transform;
    int ID;
    Material material;
};

struct Mesh {
    int triangle_sampler_index;
    int triangle_storage_side_len;
    int num_tris;

    Transform transform;
    int ID;
    Material material;
};

struct Triangle {
    vec3 pos[3];
    vec3 nor[3];
    vec2 uv[3];
};


// Functions
float AbsDot(vec3 a, vec3 b) {
    return abs(dot(a, b));
}

float CosTheta(vec3 w) { return w.z; }
float Cos2Theta(vec3 w) { return w.z * w.z; }
float AbsCosTheta(vec3 w) { return abs(w.z); }
float Sin2Theta(vec3 w) {
    return max(0.f, 1.f - Cos2Theta(w));
}
float SinTheta(vec3 w) { return sqrt(Sin2Theta(w)); }
float TanTheta(vec3 w) { return SinTheta(w) / CosTheta(w); }

float Tan2Theta(vec3 w) {
    return Sin2Theta(w) / Cos2Theta(w);
}

float CosPhi(vec3 w) {
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 1 : clamp(w.x / sinTheta, -1.f, 1.f);
}
float SinPhi(vec3 w) {
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 0 : clamp(w.y / sinTheta, -1.f, 1.f);
}
float Cos2Phi(vec3 w) { return CosPhi(w) * CosPhi(w); }
float Sin2Phi(vec3 w) { return SinPhi(w) * SinPhi(w); }

Ray SpawnRay(vec3 pos, vec3 wi) {
    return Ray(pos + wi * 0.0001, wi);
}

mat4 translate(vec3 t) {
    return mat4(1,0,0,0,
                0,1,0,0,
                0,0,1,0,
                t.x, t.y, t.z, 1);
}

float radians(float deg) {
    return deg * PI / 180.f;
}

mat4 rotateX(float rad) {
    return mat4(1,0,0,0,
                0,cos(rad),sin(rad),0,
                0,-sin(rad),cos(rad),0,
                0,0,0,1);
}

mat4 rotateY(float rad) {
    return mat4(cos(rad),0,-sin(rad),0,
                0,1,0,0,
                sin(rad),0,cos(rad),0,
                0,0,0,1);
}


mat4 rotateZ(float rad) {
    return mat4(cos(rad),sin(rad),0,0,
                -sin(rad),cos(rad),0,0,
                0,0,1,0,
                0,0,0,1);
}

mat4 scale(vec3 s) {
    return mat4(s.x,0,0,0,
                0,s.y,0,0,
                0,0,s.z,0,
                0,0,0,1);
}

Transform makeTransform(vec3 t, vec3 euler, vec3 s) {
    mat4 T = translate(t)
             * rotateX(radians(euler.x))
             * rotateY(radians(euler.y))
             * rotateZ(radians(euler.z))
             * scale(s);

    return Transform(T, inverse(T), inverse(transpose(mat3(T))), s);
}

bool Refract(vec3 wi, vec3 n, float eta, out vec3 wt) {
    // Compute cos theta using Snell's law
    float cosThetaI = dot(n, wi);
    float sin2ThetaI = max(float(0), float(1 - cosThetaI * cosThetaI));
    float sin2ThetaT = eta * eta * sin2ThetaI;

    // Handle total internal reflection for transmission
    if (sin2ThetaT >= 1) return false;
    float cosThetaT = sqrt(1 - sin2ThetaT);
    wt = eta * -wi + (eta * cosThetaI - cosThetaT) * n;
    return true;
}

vec3 Faceforward(vec3 n, vec3 v) {
    return (dot(n, v) < 0.f) ? -n : n;
}

bool SameHemisphere(vec3 w, vec3 wp) {
    return w.z * wp.z > 0;
}

void coordinateSystem(in vec3 v1, out vec3 v2, out vec3 v3) {
    if (abs(v1.x) > abs(v1.y))
            v2 = vec3(-v1.z, 0, v1.x) / sqrt(v1.x * v1.x + v1.z * v1.z);
        else
            v2 = vec3(0, v1.z, -v1.y) / sqrt(v1.y * v1.y + v1.z * v1.z);
        v3 = cross(v1, v2);
}

mat3 LocalToWorld(vec3 nor) {
    vec3 tan, bit;
    coordinateSystem(nor, tan, bit);
    return mat3(tan, bit, nor);
}

mat3 WorldToLocal(vec3 nor) {
    return transpose(LocalToWorld(nor));
}

float DistanceSquared(vec3 p1, vec3 p2) {
    return dot(p1 - p2, p1 - p2);
}



// from ShaderToy https://www.shadertoy.com/view/4tXyWN
uvec2 seed;
float rng() {
    seed += uvec2(1);
    uvec2 q = 1103515245U * ( (seed >> 1U) ^ (seed.yx) );
    uint  n = 1103515245U * ( (q.x) ^ (q.y >> 3U) );
    return float(n) * (1.0 / float(0xffffffffU));
}

#define N_TEXTURES 1
#define N_BOXES 0
#define N_RECTANGLES 1
#define N_SPHERES 3
#define N_MESHES 0
#define N_TRIANGLES 0
#define N_LIGHTS 0
#define N_AREA_LIGHTS 0
#define N_POINT_LIGHTS 0
#define N_SPOT_LIGHTS 0
uniform sampler2D u_TexSamplers[N_TEXTURES];
const Rectangle rectangles[N_RECTANGLES] = Rectangle[](Rectangle(vec3(0, 0, 0), vec3(0, 0, 1), vec2(0.5, 0.5), Transform(mat4(50, 0, 0, 0, 0, 6.33795e-05, -50, 0, 0, 3, 3.80277e-06, 0, 0, -3, 0, 1), mat4(0.02, 0, 0, 0, 0, 2.53518e-08, 0.333333, 0, 0, -0.02, 4.2253e-07, 0, 0, 7.60554e-08, 1, 1), mat3(0.02, 0, 0, 0, 2.53518e-08, -0.02, 0, 0.333333, 4.2253e-07), vec3(50, 50, 3)), 0, Material(vec3(0.1, 0.1, 0.1), 0.1, -1, 5, -1, -1, -1))
);
const Sphere spheres[N_SPHERES] = Sphere[](Sphere(vec3(0, 0, 0), 1, Transform(mat4(5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 1, 0, 3, 1, 0, 1), mat4(0.2, 0, 0, 0, 0, 0.2, 0, 0, 0, 0, 1, 0, -0.6, -0.2, 0, 1), mat3(0.2, 0, 0, 0, 0.2, 0, 0, 0, 1), vec3(5, 5, 1)), 1, Material(vec3(0.7, 1, 0.7), 0, 1.6, 3, -1, -1, -1)),
Sphere(vec3(0, 0, 0), 1, Transform(mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 3, 1, 0, 1), mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -3, -1, 0, 1), mat3(1, 0, 0, 0, 1, 0, 0, 0, 1), vec3(1, 1, 1)), 2, Material(vec3(0, 1, 1), 0, -1, 1, 0, -1, -1)),
Sphere(vec3(0, 0, 0), 1, Transform(mat4(1.2, 0, 0, 0, 0, 1.5, 0, 0, 0, 0, 5, 0, 0, 1, 0, 1), mat4(0.833333, 0, 0, 0, 0, 0.666667, 0, 0, 0, 0, 0.2, 0, 0, -0.666667, 0, 1), mat3(0.833333, 0, 0, 0, 0.666667, 0, 0, 0, 0.2), vec3(1.2, 1.5, 5)), 3, Material(vec3(1, 0, 0), 0, -1, 2, -1, -1, -1))
);


vec3 squareToDiskConcentric(vec2 sample) {
    vec2 uOffset = 2.0 * sample - vec2(1.0, 1.0);

    if (uOffset.x == 0.0 && uOffset.y == 0.0) {
        return vec3(0.0, 0.0, 0.0);
    }

    float theta, r;
    if (abs(uOffset.x) > abs(uOffset.y)) {
        r = uOffset.x;
        theta = PI / 4.0 * (uOffset.y / uOffset.x);
    } else {
        r = uOffset.y;
        theta = PI / 2.0 - PI / 4.0 * (uOffset.x / uOffset.y);
    }

    return r * vec3(cos(theta), sin(theta), 0.0);
}

vec3 squareToHemisphereCosine(vec2 sample) {
    vec2 disk = vec2(squareToDiskConcentric(sample));
    float x = disk.x;
    float y = disk.y;
    float zSquared = max(0.0, 1.0 - x * x - y * y);
    float z = sqrt(zSquared);
    return vec3(x, y, z);
}

float squareToHemisphereCosinePDF(vec3 sample) {
    return CosTheta(sample) * INV_PI;
}

vec3 squareToSphereUniform(vec2 sample) {
    float z = 1.0 - 2.0 * sample.x;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = TWO_PI * sample.y;
    float x = r * cos(phi);
    float y = r * sin(phi);
    return vec3(x, y, z);
}

float squareToSphereUniformPDF(vec3 sample) {
    return INV_FOUR_PI;
}

vec3 f_diffuse(vec3 albedo) {
    return albedo * INV_PI;
}

vec3 Sample_f_diffuse(vec3 albedo, vec2 xi, vec3 nor,
                      out vec3 wiW, out float pdf, out int sampledType) {
    vec3 wi = squareToHemisphereCosine(xi);
    wiW = LocalToWorld(nor) * wi;
    pdf = squareToHemisphereCosinePDF(wi);
    sampledType = DIFFUSE_REFL;
    return f_diffuse(albedo);
}

vec3 Sample_f_diffuse_trans(vec3 albedo, vec2 xi, vec3 nor, out vec3 wiW, out float pdf, out int sampledType) {
    vec3 wi = squareToHemisphereCosine(xi);
    wiW = LocalToWorld(-nor) * wi;
    pdf = squareToHemisphereCosinePDF(wi);
    sampledType = DIFFUSE_TRANS;
    return albedo * INV_PI;
}

vec3 Sample_f_specular_refl(vec3 albedo, vec3 nor, vec3 wo,
                            out vec3 wiW, out int sampledType) {
    //vec3 wi = reflect(-wo, nor); // R=I−2⋅(I⋅N)⋅N
    vec3 wi = reflect(-wo, vec3(0, 0, 1));
    wiW = LocalToWorld(nor) * wi;
    sampledType = SPEC_REFL;
    return albedo;
}

vec3 Sample_f_specular_trans(vec3 albedo, vec3 nor, vec3 wo,
                             out vec3 wiW, out int sampledType) {
    float etaA = 1.0;
    float etaB = 1.55;

    bool entering = CosTheta(wo) > 0.0;
    float etaI = entering ? etaA : etaB;
    float etaT = entering ? etaB : etaA;

    if (!Refract(wo, Faceforward(vec3(0, 0, 1), wo), etaI / etaT, wiW)) {
        sampledType = SPEC_REFL;
        return vec3(0.0);
    }
    wiW = LocalToWorld(nor) * wiW;
    sampledType = SPEC_TRANS;
    return albedo;
}

vec3 FresnelDielectricEval(float cosThetaI) {
    // We will hard-code the indices of refraction to be
    // those of glass
    float etaI = 1.;
    float etaT = 1.55;
    cosThetaI = clamp(cosThetaI, -1.f, 1.f);

    float R0 = pow((etaI - etaT) / (etaI + etaT), 2.0);
    float R = R0 + (1.0 - R0) * pow(1.0 - abs(cosThetaI), 5.0);

    return vec3(R);
}

vec3 Sample_f_glass(vec3 albedo, vec3 nor, vec2 xi, vec3 wo,
                    out vec3 wiW, out int sampledType) {
    float random = rng();
    if(random < 0.5) {
        // Have to double contribution b/c we only sample
        // reflection BxDF half the time
        vec3 R = Sample_f_specular_refl(albedo, nor, wo, wiW, sampledType);
        sampledType = SPEC_REFL;
        return 2 * FresnelDielectricEval(dot(nor, normalize(wiW))) * R;
    }
    else {
        // Have to double contribution b/c we only sample
        // transmit BxDF half the time
        vec3 T = Sample_f_specular_trans(albedo, nor, wo, wiW, sampledType);
        sampledType = SPEC_TRANS;
        return 2 * (vec3(1.) - FresnelDielectricEval(dot(nor, normalize(wiW)))) * T;
    }
}

// Below are a bunch of functions for handling microfacet materials.
// Don't worry about this for now.
vec3 Sample_wh(vec3 wo, vec2 xi, float roughness) {
    vec3 wh;

    float cosTheta = 0;
    float phi = TWO_PI * xi[1];
    // We'll only handle isotropic microfacet materials
    float tanTheta2 = roughness * roughness * xi[0] / (1.0f - xi[0]);
    cosTheta = 1 / sqrt(1 + tanTheta2);

    float sinTheta =
            sqrt(max(0.f, 1.f - cosTheta * cosTheta));

    wh = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    if (!SameHemisphere(wo, wh)) wh = -wh;

    return wh;
}

float TrowbridgeReitzD(vec3 wh, float roughness) {
    float tan2Theta = Tan2Theta(wh);
    if (isinf(tan2Theta)) return 0.f;

    float cos4Theta = Cos2Theta(wh) * Cos2Theta(wh);

    float e =
            (Cos2Phi(wh) / (roughness * roughness) + Sin2Phi(wh) / (roughness * roughness)) *
            tan2Theta;
    return 1 / (PI * roughness * roughness * cos4Theta * (1 + e) * (1 + e));
}

float Lambda(vec3 w, float roughness) {
    float absTanTheta = abs(TanTheta(w));
    if (isinf(absTanTheta)) return 0.;

    // Compute alpha for direction w
    float alpha =
            sqrt(Cos2Phi(w) * roughness * roughness + Sin2Phi(w) * roughness * roughness);
    float alpha2Tan2Theta = (roughness * absTanTheta) * (roughness * absTanTheta);
    return (-1 + sqrt(1.f + alpha2Tan2Theta)) / 2;
}

float TrowbridgeReitzG(vec3 wo, vec3 wi, float roughness) {
    return 1 / (1 + Lambda(wo, roughness) + Lambda(wi, roughness));
}

float TrowbridgeReitzPdf(vec3 wo, vec3 wh, float roughness) {
    return TrowbridgeReitzD(wh, roughness) * AbsCosTheta(wh);
}

vec3 f_microfacet_refl(vec3 albedo, vec3 wo, vec3 wi, float roughness) {
    float cosThetaO = AbsCosTheta(wo);
    float cosThetaI = AbsCosTheta(wi);
    vec3 wh = wi + wo;
    // Handle degenerate cases for microfacet reflection
    if (cosThetaI == 0 || cosThetaO == 0) return vec3(0.f);
    if (wh.x == 0 && wh.y == 0 && wh.z == 0) return vec3(0.f);
    wh = normalize(wh);
    // TODO: Handle different Fresnel coefficients
    vec3 F = vec3(1.);//fresnel->Evaluate(glm::dot(wi, wh));
    float D = TrowbridgeReitzD(wh, roughness);
    float G = TrowbridgeReitzG(wo, wi, roughness);
    return albedo * D * G * F /
            (4 * cosThetaI * cosThetaO);
}

vec3 Sample_f_microfacet_refl(vec3 albedo, vec3 nor, vec2 xi, vec3 wo, float roughness,
                              out vec3 wiW, out float pdf, out int sampledType) {
    if (wo.z == 0) return vec3(0.);

    vec3 wh = Sample_wh(wo, xi, roughness);
    vec3 wi = reflect(-wo, wh);
    wiW = LocalToWorld(nor) * wi;
    if (!SameHemisphere(wo, wi)) return vec3(0.f);

    // Compute PDF of _wi_ for microfacet reflection
    pdf = TrowbridgeReitzPdf(wo, wh, roughness) / (4 * dot(wo, wh));
    return f_microfacet_refl(albedo, wo, wi, roughness);
}

vec3 computeAlbedo(Intersection isect) {
    vec3 albedo = isect.material.albedo;
#if N_TEXTURES
    if(isect.material.albedoTex != -1) {
        albedo *= texture(u_TexSamplers[isect.material.albedoTex], isect.uv).rgb;
        albedo = pow(albedo, vec3(2.2));
    }
#endif
    return albedo;
}

vec3 computeNormal(Intersection isect) {
    vec3 nor = isect.nor;
#if N_TEXTURES
    if(isect.material.normalTex != -1) {
        vec3 localNor = texture(u_TexSamplers[isect.material.normalTex], isect.uv).rgb;
        vec3 tan, bit;
        coordinateSystem(nor, tan, bit);
        nor = mat3(tan, bit, nor) * localNor;
    }
#endif
    return nor;
}

float computeRoughness(Intersection isect) {
    float roughness = isect.material.roughness;
#if N_TEXTURES
    if(isect.material.roughnessTex != -1) {
        roughness = texture(u_TexSamplers[isect.material.roughnessTex], isect.uv).r;
    }
#endif
    return roughness;
}

// Computes the overall light scattering properties of a point on a Material,
// given the incoming and outgoing light directions.
vec3 f(Intersection isect, vec3 woW, vec3 wiW) {
    // Convert the incoming and outgoing light rays from
    // world space to local tangent space
    vec3 nor = computeNormal(isect);
    vec3 wo = WorldToLocal(nor) * woW;
    vec3 wi = WorldToLocal(nor) * wiW;

    // If the outgoing ray is parallel to the surface,
    // we know we can return black b/c the Lambert term
    // in the overall Light Transport Equation will be 0.
    if (wo.z == 0) return vec3(0.f);

    // Since GLSL does not support classes or polymorphism,
    // we have to handle each material type with its own function.
    if(isect.material.type == DIFFUSE_REFL) {
        return f_diffuse(computeAlbedo(isect));
    }
    // As we discussed in class, there is a 0% chance that a randomly
    // chosen wi will be the perfect reflection / refraction of wo,
    // so any specular material will have a BSDF of 0 when wi is chosen
    // independently of the material.
    else if(isect.material.type == SPEC_REFL ||
            isect.material.type == SPEC_TRANS ||
            isect.material.type == SPEC_GLASS) {
        return vec3(0.);
    }
    else if(isect.material.type == MICROFACET_REFL) {
        return f_microfacet_refl(computeAlbedo(isect),
                                 wo, wi,
                                 computeRoughness(isect));
    }
    // Default case, unhandled material
    else {
        return vec3(1,0,1);
    }
}

// Sample_f() returns the same values as f(), but importantly it
// only takes in a wo. Note that wiW is declared as an "out vec3";
// this means the function is intended to compute and write a wi
// in world space (the trailing "W" indicates world space).
// In other words, Sample_f() evaluates the BSDF *after* generating
// a wi based on the Intersection's material properties, allowing
// us to bias our wi samples in a way that gives more consistent
// light scattered along wo.
vec3 Sample_f(Intersection isect, vec3 woW, vec2 xi,
              out vec3 wiW, out float pdf, out int sampledType) {
    // Convert wo to local space from world space.
    // The various Sample_f()s output a wi in world space,
    // but assume wo is in local space.
    vec3 nor = computeNormal(isect);
    vec3 wo = WorldToLocal(nor) * woW;

    if(isect.material.type == DIFFUSE_REFL) {
        return Sample_f_diffuse(computeAlbedo(isect), xi, nor, wiW, pdf, sampledType);
    }
    else if(isect.material.type == SPEC_REFL) {
        pdf = 1.;
        return Sample_f_specular_refl(computeAlbedo(isect), nor, wo, wiW, sampledType);
    }
    else if(isect.material.type == SPEC_TRANS) {
        pdf = 1.;
        return Sample_f_specular_trans(computeAlbedo(isect), nor, wo, wiW, sampledType);
    }
    else if(isect.material.type == SPEC_GLASS) {
        pdf = 1.;
        return Sample_f_glass(computeAlbedo(isect), nor, xi, wo, wiW, sampledType);
    }
    else if(isect.material.type == MICROFACET_REFL) {
        return Sample_f_microfacet_refl(computeAlbedo(isect),
                                        nor, xi, wo,
                                        computeRoughness(isect),
                                        wiW, pdf,
                                        sampledType);
    }
    else if(isect.material.type == PLASTIC) {
        return vec3(1,0,1);
    }
    else if(isect.material.type == DIFFUSE_TRANS) {
        return Sample_f_diffuse_trans(computeAlbedo(isect), xi, nor, wiW, pdf, sampledType);
    }
    // Default case, unhandled material
    else {
        return vec3(1,0,1);
    }
}

// Compute the PDF of wi with respect to wo and the intersection's
// material properties.
float Pdf(Intersection isect, vec3 woW, vec3 wiW) {
    vec3 nor = computeNormal(isect);
    vec3 wo = WorldToLocal(nor) * woW;
    vec3 wi = WorldToLocal(nor) * wiW;

    if (wo.z == 0) return 0.; // The cosine of this vector would be zero

    if(isect.material.type == DIFFUSE_REFL) {
        return CosTheta(wi) * INV_PI;
    }
    else if(isect.material.type == SPEC_REFL ||
            isect.material.type == SPEC_TRANS ||
            isect.material.type == SPEC_GLASS) {
        return 0.;
    }
    else if(isect.material.type == MICROFACET_REFL) {
        vec3 wh = normalize(wo + wi);
        return TrowbridgeReitzPdf(wo, wh, computeRoughness(isect)) / (4 * dot(wo, wh));
    }
    else if(isect.material.type == DIFFUSE_TRANS) {
        float cosTheta = max(0.0, dot(-nor, wi));
        return cosTheta * INV_PI;
    }
    // Default case, unhandled material
    else {
        return 0.;
    }
}

// optimized algorithm for solving quadratic equations developed by Dr. Po-Shen Loh -> https://youtu.be/XKBX0r3J-9Y
// Adapted to root finding (ray t0/t1) for all quadric shapes (sphere, ellipsoid, cylinder, cone, etc.) by Erich Loftis
void solveQuadratic(float A, float B, float C, out float t0, out float t1) {
    float invA = 1.0 / A;
    B *= invA;
    C *= invA;
    float neg_halfB = -B * 0.5;
    float u2 = neg_halfB * neg_halfB - C;
    float u = u2 < 0.0 ? neg_halfB = 0.0 : sqrt(u2);
    t0 = neg_halfB - u;
    t1 = neg_halfB + u;
}

vec2 sphereUVMap(vec3 p) {
    float phi = atan(p.z, p.x);
    if(phi < 0) {
        phi += TWO_PI;
    }
    float theta = acos(p.y);
    return vec2(1 - phi/TWO_PI, 1 - theta / PI);
}

float sphereIntersect(Ray ray, float radius, vec3 pos, out vec3 localNor, out vec2 out_uv, mat4 invT) {
    ray.origin = vec3(invT * vec4(ray.origin, 1.));
    ray.direction = vec3(invT * vec4(ray.direction, 0.));
    float t0, t1;
    vec3 diff = ray.origin - pos;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(ray.direction, diff);
    float c = dot(diff, diff) - (radius * radius);
    solveQuadratic(a, b, c, t0, t1);
    localNor = t0 > 0.0 ? ray.origin + t0 * ray.direction : ray.origin + t1 * ray.direction;
    localNor = normalize(localNor);
    out_uv = sphereUVMap(localNor);
    return t0 > 0.0 ? t0 : t1 > 0.0 ? t1 : INFINITY;
}

float planeIntersect( vec4 pla, vec3 rayOrigin, vec3 rayDirection, mat4 invT) {
    rayOrigin = vec3(invT * vec4(rayOrigin, 1.));
    rayDirection = vec3(invT * vec4(rayDirection, 0.));
    vec3 n = pla.xyz;
    float denom = dot(n, rayDirection);

    vec3 pOrO = (pla.w * n) - rayOrigin;
    float result = dot(pOrO, n) / denom;
    return (result > 0.0) ? result : INFINITY;
}

float rectangleIntersect(vec3 pos, vec3 normal,
                         float radiusU, float radiusV,
                         vec3 rayOrigin, vec3 rayDirection,
                         out vec2 out_uv, mat4 invT) {
    rayOrigin = vec3(invT * vec4(rayOrigin, 1.));
    rayDirection = vec3(invT * vec4(rayDirection, 0.));
    float dt = dot(-normal, rayDirection);
    // use the following for one-sided rectangle
    if (dt < 0.0) return INFINITY;
    float t = dot(-normal, pos - rayOrigin) / dt;
    if (t < 0.0) return INFINITY;

    vec3 hit = rayOrigin + rayDirection * t;
    vec3 vi = hit - pos;
    vec3 U = normalize( cross( abs(normal.y) < 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0), normal ) );
    vec3 V = cross(normal, U);

    out_uv = vec2(dot(U, vi) / length(U), dot(V, vi) / length(V));
    out_uv = out_uv + vec2(0.5, 0.5);

    return (abs(dot(U, vi)) > radiusU || abs(dot(V, vi)) > radiusV) ? INFINITY : t;
}

float boxIntersect(vec3 minCorner, vec3 maxCorner,
                   mat4 invT, mat3 invTransT,
                   vec3 rayOrigin, vec3 rayDirection,
                   out vec3 normal, out bool isRayExiting,
                   out vec2 out_uv) {
        rayOrigin = vec3(invT * vec4(rayOrigin, 1.));
        rayDirection = vec3(invT * vec4(rayDirection, 0.));
        vec3 invDir = 1.0 / rayDirection;
        vec3 near = (minCorner - rayOrigin) * invDir;
        vec3 far  = (maxCorner - rayOrigin) * invDir;
        vec3 tmin = min(near, far);
        vec3 tmax = max(near, far);
        float t0 = max( max(tmin.x, tmin.y), tmin.z);
        float t1 = min( min(tmax.x, tmax.y), tmax.z);
        if (t0 > t1) return INFINITY;
        if (t0 > 0.0) // if we are outside the box
        {
                normal = -sign(rayDirection) * step(tmin.yzx, tmin) * step(tmin.zxy, tmin);
                normal = normalize(invTransT * normal);
                isRayExiting = false;
                vec3 p = t0 * rayDirection + rayOrigin;
                p = (p - minCorner) / (maxCorner - minCorner);
                out_uv = p.xy;
                return t0;
        }
        if (t1 > 0.0) // if we are inside the box
        {
                normal = -sign(rayDirection) * step(tmax, tmax.yzx) * step(tmax, tmax.zxy);
                normal = normalize(invTransT * normal);
                isRayExiting = true;
                vec3 p = t1 * rayDirection + rayOrigin;
                p = (p - minCorner) / (maxCorner - minCorner);
                out_uv = p.xy;
                return t1;
        }
        return INFINITY;
}

// Möller–Trumbore intersection
float triangleIntersect(vec3 p0, vec3 p1, vec3 p2,
                        vec3 rayOrigin, vec3 rayDirection) {
    const float EPSILON = 0.0000001;
    vec3 edge1, edge2, h, s, q;
    float a,f,u,v;
    edge1 = p1 - p0;
    edge2 = p2 - p0;
    h = cross(rayDirection, edge2);
    a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON) {
        return INFINITY;    // This ray is parallel to this triangle.
    }
    f = 1.0/a;
    s = rayOrigin - p0;
    u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return INFINITY;
    q = cross(s, edge1);
    v = f * dot(rayDirection, q);
    if (v < 0.0 || u + v > 1.0) {
        return INFINITY;
    }
    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * dot(edge2, q);
    if (t > EPSILON) {
        return t;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return INFINITY;
}

vec3 barycentric(vec3 p, vec3 t1, vec3 t2, vec3 t3) {
    vec3 edge1 = t2 - t1;
    vec3 edge2 = t3 - t2;
    float S = length(cross(edge1, edge2));

    edge1 = p - t2;
    edge2 = p - t3;
    float S1 = length(cross(edge1, edge2));

    edge1 = p - t1;
    edge2 = p - t3;
    float S2 = length(cross(edge1, edge2));

    edge1 = p - t1;
    edge2 = p - t2;
    float S3 = length(cross(edge1, edge2));

    return vec3(S1 / S, S2 / S, S3 / S);
}

#if N_MESHES
float meshIntersect(int mesh_id,
                    vec3 rayOrigin, vec3 rayDirection,
                    out vec3 out_nor, out vec2 out_uv,
                    mat4 invT) {

    rayOrigin = vec3(invT * vec4(rayOrigin, 1.));
    rayDirection = vec3(invT * vec4(rayDirection, 0.));

    int sampIdx = 0;// meshes[mesh_id].triangle_sampler_index;

    float t = INFINITY;

    // Iterate over each triangle, and
    // convert it to a pixel coordinate
    for(int i = 0; i < meshes[mesh_id].num_tris; ++i) {
        // pos0, pos1, pos2, nor0, nor1, nor2, uv0, uv1, uv2
        // Each triangle takes up 9 pixels
        Triangle tri;
        int first_pixel = i * 9;
        // Positions
        for(int p = first_pixel; p < first_pixel + 3; ++p) {
            int row = int(floor(float(p) / meshes[mesh_id].triangle_storage_side_len));
            int col = p - row * meshes[mesh_id].triangle_storage_side_len;

            tri.pos[p - first_pixel] = texelFetch(u_TriangleStorageSamplers[sampIdx],
                                                ivec2(col, row), 0).rgb;
        }
        first_pixel += 3;
        // Normals
        for(int n = first_pixel; n < first_pixel + 3; ++n) {
            int row = int(floor(float(n) / meshes[mesh_id].triangle_storage_side_len));
            int col = n - row * meshes[mesh_id].triangle_storage_side_len;

            tri.nor[n - first_pixel] = texelFetch(u_TriangleStorageSamplers[sampIdx],
                                                ivec2(col, row), 0).rgb;
        }
        first_pixel += 3;
        // UVs
        for(int v = first_pixel; v < first_pixel + 3; ++v) {
            int row = int(floor(float(v) / meshes[mesh_id].triangle_storage_side_len));
            int col = v - row * meshes[mesh_id].triangle_storage_side_len;

            tri.uv[v - first_pixel] = texelFetch(u_TriangleStorageSamplers[sampIdx],
                                               ivec2(col, row), 0).rg;
        }

        float d = triangleIntersect(tri.pos[0], tri.pos[1], tri.pos[2],
                                    rayOrigin, rayDirection);
        if(d < t) {
            t = d;
            vec3 p = rayOrigin + t * rayDirection;
            vec3 baryWeights = barycentric(p, tri.pos[0], tri.pos[1], tri.pos[2]);
            out_nor = baryWeights[0] * tri.nor[0] +
                      baryWeights[1] * tri.nor[1] +
                      baryWeights[2] * tri.nor[2];
            out_uv =  baryWeights[0] * tri.uv[0] +
                      baryWeights[1] * tri.uv[1] +
                      baryWeights[2] * tri.uv[2];
        }
    }

    return t;
}
#endif

Intersection sceneIntersect(Ray ray) {
    float t = INFINITY;
    Intersection result;
    result.t = INFINITY;

#if N_RECTANGLES
    for(int i = 0; i < N_RECTANGLES; ++i) {
        vec2 uv;
        float d = rectangleIntersect(rectangles[i].pos, rectangles[i].nor,
                                     rectangles[i].halfSideLengths.x,
                                     rectangles[i].halfSideLengths.y,
                                     ray.origin, ray.direction,
                                     uv,
                                     rectangles[i].transform.invT);
        if(d < t) {
            t = d;
            result.t = t;
            result.nor = normalize(rectangles[i].transform.invTransT * rectangles[i].nor);
            result.uv = uv;
            result.Le = vec3(0,0,0);
            result.obj_ID = rectangles[i].ID;
            result.material = rectangles[i].material;
        }
    }
#endif
#if N_BOXES
    for(int i = 0; i < N_BOXES; ++i) {
        vec3 nor;
        bool isExiting;
        vec2 uv;
        float d = boxIntersect(boxes[i].minCorner, boxes[i].maxCorner,
                               boxes[i].transform.invT, boxes[i].transform.invTransT,
                               ray.origin, ray.direction,
                               nor, isExiting, uv);
        if(d < t) {
            t = d;
            result.t = t;
            result.nor = nor;
            result.Le = vec3(0,0,0);
            result.obj_ID = boxes[i].ID;
            result.material = boxes[i].material;
            result.uv = uv;
        }
    }
#endif
#if N_SPHERES
    for(int i = 0; i < N_SPHERES; ++i) {
        vec3 nor;
        bool isExiting;
        vec3 localNor;
        vec2 uv;
        float d = sphereIntersect(ray, spheres[i].radius, spheres[i].pos, localNor, uv,
                                  spheres[i].transform.invT);
        if(d < t) {
            t = d;
            vec3 p = ray.origin + t * ray.direction;
            result.t = t;
            result.nor = normalize(spheres[i].transform.invTransT * localNor);
            result.Le = vec3(0,0,0);
            result.uv = uv;
            result.obj_ID = spheres[i].ID;
            result.material = spheres[i].material;
        }
    }
#endif
#if N_MESHES
    for(int i = 0; i < N_MESHES; ++i) {
        vec3 nor;
        vec2 uv;
        float d = meshIntersect(i, ray.origin, ray.direction,
                                nor, uv, meshes[i].transform.invT);

        if(d < t) {
            t = d;
            result.t = t;
            result.nor = nor;
            result.uv =  uv;
            result.Le = vec3(0,0,0);
            result.obj_ID = meshes[i].ID;
            result.material = meshes[i].material;
        }
    }
#endif
#if N_AREA_LIGHTS
    for(int i = 0; i < N_AREA_LIGHTS; ++i) {
        int shapeType = areaLights[i].shapeType;
        if(shapeType == RECTANGLE) {
            vec3 pos = vec3(0,0,0);
            vec3 nor = vec3(0,0,1);
            vec2 halfSideLengths = vec2(0.5, 0.5);
            vec2 uv;
            float d = rectangleIntersect(pos, nor,
                                   halfSideLengths.x,
                                   halfSideLengths.y,
                                   ray.origin, ray.direction,
                                   uv,
                                   areaLights[i].transform.invT);
            if(d < t) {
                t = d;
                result.t = t;
                result.nor = normalize(areaLights[i].transform.invTransT * vec3(0,0,1));
                result.Le = areaLights[i].Le;
                result.obj_ID = areaLights[i].ID;
            }
        }
        else if(shapeType == SPHERE) {
            vec3 pos = vec3(0,0,0);
            float radius = 1.;
            mat4 invT = areaLights[i].transform.invT;
            vec3 localNor;
            vec2 uv;
            float d = sphereIntersect(ray, radius, pos, localNor, uv, invT);
            if(d < t) {
                t = d;
                result.t = t;
                result.nor = normalize(areaLights[i].transform.invTransT * localNor);
                result.Le = areaLights[i].Le;
                result.obj_ID = areaLights[i].ID;
            }
        }
    }
#endif
#if N_TEXTURES
    if(result.material.normalTex != -1) {
        vec3 localNor = texture(u_TexSamplers[result.material.normalTex], result.uv).rgb;
        localNor = localNor * 2. - vec3(1.);
        vec3 tan, bit;
        coordinateSystem(result.nor, tan, bit);
        result.nor = mat3(tan, bit, result.nor) * localNor;
    }
#endif
    return result;
}

Intersection areaLightIntersect(AreaLight light, Ray ray) {
    Intersection result;
    result.t = INFINITY;
#if N_AREA_LIGHTS
    int shapeType = light.shapeType;
    if(shapeType == RECTANGLE) {
        vec3 pos = vec3(0,0,0);
        vec3 nor = vec3(0,0,1);
        vec2 halfSideLengths = vec2(0.5, 0.5);
        vec2 uv;
        float d = rectangleIntersect(pos, nor,
                               halfSideLengths.x,
                               halfSideLengths.y,
                               ray.origin, ray.direction,
                               uv,
                               light.transform.invT);
        result.t = d;
        result.nor = normalize(light.transform.invTransT * vec3(0,0,1));
        result.Le = light.Le;
        result.obj_ID = light.ID;
    }
    else if(shapeType == SPHERE) {
        vec3 pos = vec3(0,0,0);
        float radius = 1.;
        mat4 invT = light.transform.invT;
        vec3 localNor;
        vec2 uv;
        float d = sphereIntersect(ray, radius, pos, localNor, uv, invT);
        result.t = d;
        result.nor = normalize(light.transform.invTransT * localNor);
        result.Le = light.Le;
        result.obj_ID = light.ID;
    }
#endif
    return result;
}


vec2 normalize_uv = vec2(0.1591, 0.3183);
vec2 sampleSphericalMap(vec3 v) {
    // U is in the range [-PI, PI], V is [-PI/2, PI/2]
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    // Convert UV to [-0.5, 0.5] in U&V
    uv *= normalize_uv;
    // Convert UV to [0, 1]
    uv += 0.5;
    return uv;
}

vec3 sampleFromInsideSphere(vec2 xi, out float pdf) {

    return vec3(0.);
}

#if N_AREA_LIGHTS
vec3 DirectSampleAreaLight(int idx,
                           vec3 view_point, vec3 view_nor,
                           int num_lights,
                           out vec3 wiW, out float pdf) {
    AreaLight light = areaLights[idx];
    int type = light.shapeType;
    Ray shadowRay;

    if (type == RECTANGLE) {
        // Compute a random point on the surface of the AreaLight
        vec2 randomPoint = vec2(rng(), rng()) * 2.0 - vec2(1.0); // Random point in [-1, 1]
        vec4 worldPoint = light.transform.T * vec4(randomPoint.x, randomPoint.y, 0.0, 1.0);

        wiW = normalize(worldPoint.xyz - view_point);

        float r = length(worldPoint.xyz - view_point);

        vec3 lightNormal = normalize((light.transform.T[2]).xyz);

        float cosTheta = dot(-wiW, lightNormal);

        if (cosTheta <= 0.0) {
            pdf = 0.0;
            return vec3(0.0);
        }

        float area = 2 * 2 * light.transform.scale.x * light.transform.scale.y;
        pdf = r * r / (abs(cosTheta) * area);

        shadowRay = SpawnRay(view_point, wiW);
        if (sceneIntersect(shadowRay).obj_ID != light.ID) {
            return vec3(0.0);
        }

        return light.Le * float(num_lights);
    }
    else if(type == SPHERE) {
        Transform tr = areaLights[idx].transform;

        vec2 xi = vec2(rng(), rng());

        vec3 center = vec3(tr.T * vec4(0., 0., 0., 1.));
        vec3 centerToRef = normalize(center - view_point);
        vec3 tan, bit;

        coordinateSystem(centerToRef, tan, bit);

        vec3 pOrigin;
        if(dot(center - view_point, view_nor) > 0) {
            pOrigin = view_point + view_nor * RayEpsilon;
        }
        else {
            pOrigin = view_point - view_nor * RayEpsilon;
        }

        // Inside the sphere
        if(dot(pOrigin - center, pOrigin - center) <= 1.f) // Radius is 1, so r^2 is also 1
            return sampleFromInsideSphere(xi, pdf);

        float sinThetaMax2 = 1 / dot(view_point - center, view_point - center); // Again, radius is 1
        float cosThetaMax = sqrt(max(0.0f, 1.0f - sinThetaMax2));
        float cosTheta = (1.0f - xi.x) + xi.x * cosThetaMax;
        float sinTheta = sqrt(max(0.f, 1.0f- cosTheta * cosTheta));
        float phi = xi.y * TWO_PI;

        float dc = distance(view_point, center);
        float ds = dc * cosTheta - sqrt(max(0.0f, 1 - dc * dc * sinTheta * sinTheta));

        float cosAlpha = (dc * dc + 1 - ds * ds) / (2 * dc * 1);
        float sinAlpha = sqrt(max(0.0f, 1.0f - cosAlpha * cosAlpha));

        vec3 nObj = sinAlpha * cos(phi) * -tan + sinAlpha * sin(phi) * -bit + cosAlpha * -centerToRef;
        vec3 pObj = vec3(nObj); // Would multiply by radius, but it is always 1 in object space

        shadowRay = SpawnRay(view_point, normalize(vec3(tr.T * vec4(pObj, 1.0f)) - view_point));
        wiW = shadowRay.direction;
        pdf = 1.0f / (TWO_PI * (1 - cosThetaMax));
        pdf /= tr.scale.x * tr.scale.x;
    }

    Intersection isect = sceneIntersect(shadowRay);
    if(isect.obj_ID == areaLights[idx].ID) {
        // Multiply by N+1 to account for sampling it 1/(N+1) times.
        // +1 because there's also the environment light
        return num_lights * areaLights[idx].Le;
    }
}
#endif

#if N_POINT_LIGHTS
vec3 DirectSamplePointLight(int idx,
                            vec3 view_point, int num_lights,
                            out vec3 wiW, out float pdf) {
    PointLight light = pointLights[idx];
    wiW = normalize(light.pos - view_point);
    pdf = 1.0;

    float distance = length(light.pos - view_point);

    Ray shadowRay;
    shadowRay = SpawnRay(view_point, wiW);

    Intersection shadowIntersect = sceneIntersect(shadowRay);
    if (shadowIntersect.t < distance && shadowIntersect.t > 0.0) {
        return vec3(0.0);
    }
    return light.Le / (distance * distance) * float(num_lights);
}
#endif

#if N_SPOT_LIGHTS
vec3 DirectSampleSpotLight(int idx,
                           vec3 view_point, int num_lights,
                           out vec3 wiW, out float pdf) {
    SpotLight light = spotLights[idx];

    vec3 lightPos = light.transform.T[3].xyz;
    wiW = normalize(lightPos - view_point);

    float distance = length(lightPos - view_point);

    float cosOuter = cos(radians(light.outerAngle));
    float cosInner = cos(radians(light.innerAngle));
    vec3 lightNor = normalize(vec3(light.transform.invTransT * vec3(0.0, 0.0, 1.0)));
    float cosAngle = dot(-wiW, lightNor);

    if (cosAngle < cosOuter) {
        return vec3(0.0);
    }

    Ray shadowRay = SpawnRay(view_point, wiW);
    if (sceneIntersect(shadowRay).t < distance) {
        return vec3(0.0);
    }

    vec3 le = light.Le;
    if (cosAngle < cosInner) {
        le = smoothstep(cosOuter, cosInner, cosAngle) * light.Le;
    }
    pdf = 1.0;
    return le / (distance * distance) * num_lights;
}
#endif

vec3 Sample_Li(vec3 view_point, vec3 nor,
               out vec3 wiW, out float pdf,
               out int chosenLightIdx,
               out int chosenLightID,
               out bool isPointOrSpotLight) {
    // Choose a random light from among all of the
    // light sources in the scene, including the environment light
    //int num_lights = N_LIGHTS;
#define ENV_MAP 1
#if ENV_MAP
    int num_lights = N_LIGHTS + 1;
#endif
    int randomLightIdx = int(rng() * num_lights);
    chosenLightIdx = randomLightIdx;

    isPointOrSpotLight = (randomLightIdx >= N_AREA_LIGHTS) &&
                             (randomLightIdx < N_AREA_LIGHTS + N_POINT_LIGHTS ||
                              randomLightIdx < N_AREA_LIGHTS + N_POINT_LIGHTS + N_SPOT_LIGHTS);


    // Chose an area light
    if(randomLightIdx < N_AREA_LIGHTS) {
#if N_AREA_LIGHTS
        chosenLightID = areaLights[chosenLightIdx].ID;
        return DirectSampleAreaLight(randomLightIdx, view_point, nor, num_lights, wiW, pdf);
#endif
    }
    // Chose a point light
    else if(randomLightIdx < N_AREA_LIGHTS + N_POINT_LIGHTS) {
#if N_POINT_LIGHTS
        chosenLightID = pointLights[randomLightIdx - N_AREA_LIGHTS].ID;
        return DirectSamplePointLight(randomLightIdx - N_AREA_LIGHTS, view_point, num_lights, wiW, pdf);
#endif
    }
    // Chose a spot light
    else if(randomLightIdx < N_AREA_LIGHTS + N_POINT_LIGHTS + N_SPOT_LIGHTS) {
#if N_SPOT_LIGHTS
        chosenLightID = spotLights[randomLightIdx - N_AREA_LIGHTS - N_POINT_LIGHTS].ID;
        return DirectSampleSpotLight(randomLightIdx - N_AREA_LIGHTS - N_POINT_LIGHTS, view_point, num_lights, wiW, pdf);
#endif
    }
    // Chose the environment light
    else {
        chosenLightID = -1;

        vec2 xi = vec2(rng(), rng());
        vec3 sampleDir = squareToHemisphereCosine(xi);

        mat3 localToWorldMatrix = LocalToWorld(nor);
        wiW = localToWorldMatrix * sampleDir;

        vec2 uv = sampleSphericalMap(wiW);
        vec3 envColor = texture(u_EnvironmentMap, uv).rgb;

        pdf = squareToHemisphereCosinePDF(wiW);

        return envColor;
    }
    return vec3(0.);
}

float UniformConePdf(float cosThetaMax) {
    return 1 / (2 * PI * (1 - cosThetaMax));
}

float SpherePdf(Intersection ref, vec3 p, vec3 wi,
                Transform transform, float radius) {
    vec3 nor = ref.nor;
    vec3 pCenter = (transform.T * vec4(0, 0, 0, 1)).xyz;
    // Return uniform PDF if point is inside sphere
    vec3 pOrigin = p + nor * 0.0001;
    // If inside the sphere
    if(DistanceSquared(pOrigin, pCenter) <= radius * radius) {
//        return Shape::Pdf(ref, wi);
        // To be provided later
        return 0.f;
    }

    // Compute general sphere PDF
    float sinThetaMax2 = radius * radius / DistanceSquared(p, pCenter);
    float cosThetaMax = sqrt(max(0.f, 1.f - sinThetaMax2));
    return UniformConePdf(cosThetaMax);
}

float Pdf_Li(vec3 view_point, vec3 nor, vec3 wiW, int chosenLightIdx) {

    Ray ray = SpawnRay(view_point, wiW);

    // Area light
    if(chosenLightIdx < N_AREA_LIGHTS) {
#if N_AREA_LIGHTS
        Intersection isect = areaLightIntersect(areaLights[chosenLightIdx],
                                                ray);
        if(isect.t == INFINITY) {
            return 0.;
        }
        vec3 light_point = ray.origin + isect.t * wiW;
        // If doesn't intersect, 0 PDF
        if(isect.t == INFINITY) {
            return 0.;
        }

        int type = areaLights[chosenLightIdx].shapeType;
        if(type == RECTANGLE) {
            float pdf;
            float r = length(light_point  - view_point);

            vec3 lightNormal = normalize((areaLights[chosenLightIdx].transform.T[2]).xyz);

            float cosTheta = dot(-wiW, lightNormal);

            if (cosTheta <= 0.0) {
                pdf = 0.0;
                return 0.0;
            }

            float area = 2 * 2 * areaLights[chosenLightIdx].transform.scale.x * areaLights[chosenLightIdx].transform.scale.y;
            pdf = r * r / (cosTheta) * area;
            return pdf;
        }
        else if(type == SPHERE) {
            return SpherePdf(isect, light_point, wiW,
                                  areaLights[chosenLightIdx].transform,
                                  1.f);
        }
#endif
    }
    // Point light or spot light
    else if(chosenLightIdx < N_AREA_LIGHTS + N_POINT_LIGHTS ||
            chosenLightIdx < N_AREA_LIGHTS + N_POINT_LIGHTS + N_SPOT_LIGHTS) {
        return 0;
    }
    // Env map
    else {
        Intersection isect = sceneIntersect(ray);
        if(isect.t == INFINITY) {
            return INV_FOUR_PI;
        }
        else {
            return 0.0f;
        }
    }
}

float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
    float f = float(nf) * fPdf;
    float g = float(ng) * gPdf;
    return (f * f) / (f * f + g * g);
}

vec3 MIS_DirectLight(Intersection isect, vec3 wo, Ray ray) {
    if(dot(isect.Le, isect.Le) > 0) {
        return isect.Le; // If the intersection point itself is a light source
    }

    vec3 p = ray.origin + ray.direction * isect.t;
    vec3 n = isect.nor; // The normal at the intersection

    // Light sampling
    vec3 wiL;
    float pdfL_L, pdfL_B;
    int chosenLightIdx, chosenLightID;
    bool isPointOrSpotLight;
    vec3 LiL = Sample_Li(p, n, wiL, pdfL_L, chosenLightIdx, chosenLightID, isPointOrSpotLight);

    if(pdfL_L <= 0.01) {
        LiL = vec3(0.0);
    }
    pdfL_B = Pdf(isect, wo, wiL);
    vec3 fL = f(isect, wo, wiL);

    if (length(fL) < 0.01) {
        return vec3(0.0);
    }

    float absDotL = AbsDot(wiL, n);

    // BSDF sampling
    vec3 wiB;
    float pdfB_B, pdfB_L;
    int sampledTypeB;
    vec3 fB = Sample_f(isect, wo, vec2(rng(), rng()), wiB, pdfB_B, sampledTypeB);
    if (pdfB_B <= 0.01) {
        return vec3(0.0);
    }

    float absDotB = AbsDot(wiB, n);
    vec3 LiB = vec3(0.0);
    if (pdfB_B > 0.01) {
        Ray bsdfRay = SpawnRay(p + n * RayEpsilon, wiB);
        Intersection bsdfIsect = sceneIntersect(bsdfRay);
        if (bsdfIsect.obj_ID == chosenLightID && bsdfIsect.Le != vec3(0.0)) {
            LiB = bsdfIsect.Le;
        }
    }
    pdfB_L = Pdf_Li(p, n, wiB, chosenLightIdx);

    // Directly return the Lo for point lights or spot lights
    if (isPointOrSpotLight) {
        return fL * LiL * absDotL;
    }
    // Combine the samples using the power heuristic
    vec3 Lo = vec3(0.0);
    if (LiL != vec3(0.0)) {
        Lo += fL * LiL * absDotL / pdfL_L * PowerHeuristic(1, pdfL_L, 1, pdfL_B);
    }
    if (LiB != vec3(0.0)) {
        Lo += fB * LiB * absDotB / pdfB_B * PowerHeuristic(1, pdfB_B, 1, pdfB_L);
    }

    // Check for NaN values in the final color
    if (any(isnan(Lo))) {
        return vec3(0.0);
    }
    return Lo;
}


const float FOVY = 19.5f * PI / 180.0;


Ray rayCast() {
    vec2 offset = vec2(rng(), rng());
    vec2 ndc = (vec2(gl_FragCoord.xy) + offset) / vec2(u_ScreenDims);
    ndc = ndc * 2.f - vec2(1.f);

    float aspect = u_ScreenDims.x / u_ScreenDims.y;
    vec3 ref = u_Eye + u_Forward;
    vec3 V = u_Up * tan(FOVY * 0.5);
    vec3 H = u_Right * tan(FOVY * 0.5) * aspect;
    vec3 p = ref + H * ndc.x + V * ndc.y;

    return Ray(u_Eye, normalize(p - u_Eye));
}


vec3 Li_Naive(Ray ray) {
    vec3 color = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int bounce = 0; bounce < MAX_DEPTH; bounce++) {
        Intersection p = sceneIntersect(ray);
        if (p.t < INFINITY) {
            if (p.Le != vec3(0.0)) { // Light emission
                color += throughput * p.Le;
                break;
            }

            vec3 hitPoint = ray.origin + ray.direction * p.t;
            vec2 xi = vec2(rng(), rng());
            vec3 wiW;
            float pdf;
            int sampledType;

            // Sample the BSDF to get new direction and PDF
            vec3 bsdf = Sample_f(p, -ray.direction, xi, wiW, pdf, sampledType);
            if (pdf <= 0.0) {
                break;
            }
            // Compute the cosine term based on the material type
            float cosTheta = 0.0;
            if (sampledType == DIFFUSE_REFL || sampledType == MICROFACET_REFL) {
                cosTheta = max(0.0, dot(wiW, p.nor));
            } else if (sampledType == DIFFUSE_TRANS) {
                cosTheta = max(0.0, dot(wiW, -p.nor));
            } else if (sampledType == SPEC_REFL || sampledType == SPEC_TRANS || sampledType == SPEC_GLASS) {
                cosTheta = 1.0;
            }
            else {
                cosTheta = max(0.0, dot(wiW, p.nor));
            }
            throughput *= bsdf * cosTheta / pdf;

            // Handle case for Specular Trans & Diffuse Trans
            vec3 offset = ((sampledType == SPEC_TRANS || sampledType == SPEC_GLASS) && dot(wiW, p.nor) >= 0.0) ? -p.nor : p.nor;
            ray = SpawnRay(hitPoint + offset * RayEpsilon, wiW);
        } else {
            // If no intersection, break from the loop
            break;
        }
    }

    return color;
}

// Li_Naive code from lecture
vec3 Li_Naive2(Ray ray) {
    vec3 accumLight = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int bounces = 0; bounces < MAX_DEPTH; ++bounces) {
        Intersection isect = sceneIntersect(ray);
        vec3 wo = -ray.direction;
        int sampledType;

        if (isect.t == INFINITY) {
            if (bounces == 0) {
            }
            break;
        }
        vec3 p = ray.origin + isect.t * ray.direction;
        if (dot(isect.Le, isect.Le) > 0.0) {
            accumLight += isect.Le * throughput;
            break;
        } else {
            vec3 wi;
            float pdf;
            vec2 xi = vec2(rng(), rng());
            vec3 f = Sample_f(isect, wo, xi, wi, pdf, sampledType);
            if (pdf > 0) {
                throughput *= f * AbsDot(wi, isect.nor) / pdf;
            } else {
                return vec3(0.0);
            }
            ray = SpawnRay(p, wi);
        }
    }
    return accumLight;
}

vec3 Li_Direct_Simple(Ray ray) {
    vec3 color = vec3(0.0);
    Intersection p = sceneIntersect(ray);

    if (p.t < INFINITY) {
        if (p.Le != vec3(0.0)) {
            return p.Le;
        }

        vec3 hitPoint = ray.origin + ray.direction * p.t;

        vec3 wiW;
        float pdf;
        int chosenLightIdx, chosenLightID;
        bool isPointOrSpotLight;
        vec3 Li = Sample_Li(hitPoint, p.nor, wiW, pdf, chosenLightIdx, chosenLightID, isPointOrSpotLight);
        if (pdf > 0.0 && length(wiW) != 0.0) {
            vec3 f = f(p, -ray.direction, wiW);

            color += f * Li * abs(dot(wiW, p.nor)) / pdf;
        }
    }

    return color;
}

vec3 Li_DirectMIS(Ray ray)
{
    Intersection isect = sceneIntersect(ray);
    if (isect.t == INFINITY) {
        return vec3(0.0);
    }

    if(dot(isect.Le, isect.Le) > 0) {
        return isect.Le;
    }
    vec3 p = ray.origin + isect.t * ray.direction;
    vec3 wo = -ray.direction;

    // Light sampling
    vec3 wiL;
    float pdfL_L, pdfL_B;
    int chosenLightIdx, chosenLightID;
    bool isPointOrSpotLight;
    vec3 LiL = Sample_Li(p, isect.nor, wiL, pdfL_L, chosenLightIdx, chosenLightID, isPointOrSpotLight);
    if(pdfL_L <= 0.01) {
        LiL = vec3(0.f);
    }
    pdfL_B = Pdf(isect, wo, wiL);

    vec3 fL = f(isect, wo, wiL);
    if (length(fL) < 0.01) {
            return vec3(0.0);
        }
    float absDotL = AbsDot(wiL, isect.nor);
    vec3 LiB = vec3(0.0);

    // BSDF sampling
    vec3 wiB;
    float pdfB_B, pdfB_L;
    int sampledTypeB;
    vec3 fB = Sample_f(isect, wo, vec2(rng(), rng()), wiB, pdfB_B, sampledTypeB);
    if (pdfB_B < 0.01) {
        return vec3(0.0);
    }

    float absDotB = AbsDot(wiB, isect.nor);
    if (pdfB_B >= 0.01) {
            Ray bsdfRay = SpawnRay(p, wiB);
            Intersection bsdfIsect = sceneIntersect(bsdfRay);
            if (bsdfIsect.obj_ID == chosenLightID) {
                LiB = bsdfIsect.Le;

            }
    }
    pdfB_L = Pdf_Li(p, isect.nor, wiB, chosenLightIdx);

    // Combine the samples using the power heuristic
    vec3 Lo = vec3(0.0);
    if (LiL != vec3(0)) {
        Lo += fL * LiL * absDotL / pdfL_L * PowerHeuristic(1, pdfL_L, 1, pdfL_B);
    }
    if (LiB != vec3(0)) {
        Lo += fB * LiB * absDotB / pdfB_B * PowerHeuristic(1, pdfB_B, 1, pdfB_L);
    }

    // Check for NaN values in the final color
    if (any(isnan(Lo))) {
        return vec3(0.0);
    }
    return Lo;
}

vec3 Li_Full(Ray ray) {
    vec3 color = vec3(0.0);
    vec3 throughput = vec3(1.0);
    vec3 accumulated = vec3(0.0);
    bool lastBounceSpecular = false;

    for (int bounce = 0; bounce < MAX_DEPTH; bounce++) {
        Intersection p = sceneIntersect(ray);

        if (p.t == INFINITY) {
            vec2 uv = sampleSphericalMap(ray.direction);
            accumulated += throughput * texture(u_EnvironmentMap, uv).rgb;
            break;
        }

        if ((p.material.type == SPEC_REFL || p.material.type == SPEC_TRANS || p.material.type == SPEC_GLASS)) {
            lastBounceSpecular = true;
        }

        // Handle light emission
        if (p.Le != vec3(0.0) && (bounce == 0 || lastBounceSpecular)) {
            accumulated += throughput * p.Le;
            break;
        }
        vec3 directLight = vec3(0.f);
        if (!lastBounceSpecular) {
            // Compute MIS Direct Light for non-specular surfaces
            directLight = MIS_DirectLight(p, -ray.direction, ray);
            accumulated += throughput * directLight;

            // Prepare for indirect light sampling
            vec3 hitPoint = ray.origin + ray.direction * p.t;
            vec2 xi = vec2(rng(), rng());
            float pdf;
            int sampledType;
            vec3 wi;

            vec3 bsdf = Sample_f(p, -ray.direction, xi, wi, pdf, sampledType);
            if (pdf > 0) {
                throughput *= bsdf * AbsDot(wi, p.nor) / pdf;
            }
            else {
                return vec3(0.0);
            }

            ray = SpawnRay(hitPoint, wi);

            lastBounceSpecular = (sampledType == SPEC_REFL || sampledType == SPEC_TRANS || sampledType == SPEC_GLASS);
        } else {
            lastBounceSpecular = true;
            vec3 hitPoint = ray.origin + ray.direction * p.t;
            vec2 xi = vec2(rng(), rng());
            float pdf;
            int sampledType;
            vec3 wi;

            vec3 bsdf = Sample_f(p, -ray.direction, xi, wi, pdf, sampledType);
            if (pdf > 0) {
                throughput *= bsdf * AbsDot(wi, p.nor) / pdf;
            }
            else {
                return vec3(0.0);
            }

            ray = SpawnRay(hitPoint, wi);
        }
    }

    return accumulated;
}

void main() {
    seed = uvec2(u_Iterations, u_Iterations + 1) * uvec2(gl_FragCoord.xy);
    Ray ray = rayCast();
    //vec3 thisIterationColor = Li_Naive(ray);
    //vec3 thisIterationColor = Li_Direct_Simple(ray);
    //vec3 thisIterationColor = Li_DirectMIS(ray);
    vec3 thisIterationColor = Li_Full(ray);


    vec3 accumulatedColor = texture(u_AccumImg, fs_UV).rgb;

    float newColorWeight = 1.0 / float(u_Iterations);
    vec3 newAccumulatedColor = mix(accumulatedColor, thisIterationColor, newColorWeight);

    out_Col = vec4(newAccumulatedColor, 1.0);
}
 
