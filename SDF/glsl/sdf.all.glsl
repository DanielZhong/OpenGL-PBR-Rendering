#version 330 core

uniform float u_Time;

uniform vec3 u_CamPos;
uniform vec3 u_Forward, u_Right, u_Up;
uniform vec2 u_ScreenDims;

// PBR material attributes
uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AmbientOcclusion;
// Texture maps for controlling some of the attribs above, plus normal mapping
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_AOMap;
uniform sampler2D u_NormalMap;
// If true, use the textures listed above instead of the GUI slider values
uniform bool u_UseAlbedoMap;
uniform bool u_UseMetallicMap;
uniform bool u_UseRoughnessMap;
uniform bool u_UseAOMap;
uniform bool u_UseNormalMap;

// Image-based lighting
uniform samplerCube u_DiffuseIrradianceMap;
uniform samplerCube u_GlossyIrradianceMap;
uniform sampler2D u_BRDFLookupTexture;

// Varyings
in vec2 fs_UV;
out vec4 out_Col;

const float PI = 3.14159f;
const float cell = 5.0;
const vec3 cellSize = vec3(cell, cell, cell);

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct BSDF {
    vec3 pos;
    vec3 nor;
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    float thinness;
};

struct MarchResult {
    float t;
    int hitSomething;
    BSDF bsdf;
};

struct SmoothMinResult {
    float dist;
    float material_t;
};

float dot2( in vec2 v ) { return dot(v,v); }
float dot2( in vec3 v ) { return dot(v,v); }
float ndot( in vec2 a, in vec2 b ) { return a.x*b.x - a.y*b.y; }

float sceneSDF(vec3 query);

vec3 SDF_Normal(vec3 query) {
    vec2 epsilon = vec2(0.0, 0.001);
    return normalize( vec3( sceneSDF(query + epsilon.yxx) - sceneSDF(query - epsilon.yxx),
                            sceneSDF(query + epsilon.xyx) - sceneSDF(query - epsilon.xyx),
                            sceneSDF(query + epsilon.xxy) - sceneSDF(query - epsilon.xxy)));
}

float SDF_Sphere(vec3 query, vec3 center, float radius) {
    return length(query - center) - radius;
}

float SDF_Box(vec3 query, vec3 bounds ) {
  vec3 q = abs(query) - bounds;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float SDF_RoundCone( vec3 query, vec3 a, vec3 b, float r1, float r2) {
  // sampling independent computations (only depend on shape)
  vec3  ba = b - a;
  float l2 = dot(ba,ba);
  float rr = r1 - r2;
  float a2 = l2 - rr*rr;
  float il2 = 1.0/l2;

  // sampling dependant computations
  vec3 pa = query - a;
  float y = dot(pa,ba);
  float z = y - l2;
  float x2 = dot2( pa*l2 - ba*y );
  float y2 = y*y*l2;
  float z2 = z*z*l2;

  // single square root!
  float k = sign(rr)*rr*rr*x2;
  if( sign(z)*a2*z2>k ) return  sqrt(x2 + z2)        *il2 - r2;
  if( sign(y)*a2*y2<k ) return  sqrt(x2 + y2)        *il2 - r1;
                        return (sqrt(x2*a2*il2)+y*rr)*il2 - r1;
}

float smooth_min( float a, float b, float k ) {
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * k * 0.25;
}

SmoothMinResult smooth_min_lerp( float a, float b, float k ) {
    float h = max( k-abs(a-b), 0.0 )/k;
    float m = h*h*0.5;
    float s = m*k*0.5;
    if(a < b) {
        return SmoothMinResult(a-s,m);
    }
    return SmoothMinResult(b-s,1.0-m);
}
vec3 repeat(vec3 query, vec3 cell) {
    return mod(query + 0.5 * cell, cell) - 0.5 * cell;
}

float subtract(float d1, float d2) {
    return max(d1, -d2);
}

float opIntersection( float d1, float d2 ) {
    return max(d1,d2);
}

float opOnion(float sdf, float thickness ) {
    return abs(sdf)-thickness;
}

vec3 rotateX(vec3 p, float angle) {
    angle = angle * 3.14159 / 180.f;
    float c = cos(angle);
    float s = sin(angle);
    return vec3(p.x, c * p.y - s * p.z, s * p.y + c * p.z);
}

vec3 rotateZ(vec3 p, float angle) {
    angle = angle * 3.14159 / 180.f;
    float c = cos(angle);
    float s = sin(angle);
    return vec3(c * p.x - s * p.y, s * p.x + c * p.y, p.z);
}

float SDF_Stache(vec3 query) {
    float left = SDF_Sphere(query / vec3(1,1,0.3), vec3(0.2, -0.435, 3.5), 0.1) * 0.1;
    left = min(left, SDF_Sphere(query / vec3(1,1,0.3), vec3(0.45, -0.355, 3.5), 0.1) * 0.1);
    left = min(left, SDF_Sphere(query / vec3(1,1,0.3), vec3(0.7, -0.235, 3.5), 0.09) * 0.1);
    left = subtract(left, SDF_Sphere(rotateZ(query, -15) / vec3(1.3,1,1), vec3(0.3, -0.1, 1.), 0.35));

    float right = SDF_Sphere(query / vec3(1,1,0.3), vec3(-0.2, -0.435, 3.5), 0.1) * 0.1;
    right = min(right, SDF_Sphere(query / vec3(1,1,0.3), vec3(-0.45, -0.355, 3.5), 0.1) * 0.1);
    right = min(right, SDF_Sphere(query / vec3(1,1,0.3), vec3(-0.7, -0.235, 3.5), 0.09) * 0.1);
    right = subtract(right, SDF_Sphere(rotateZ(query, 15) / vec3(1.3,1,1), vec3(-0.3, -0.1, 1.), 0.35));

    return min(left, right);
}

float SDF_Wahoo_Skin(vec3 query) {
    // head base
    float result = SDF_Sphere(query / vec3(1,1.2,1), vec3(0,0,0), 1.) * 1.1;
    // cheek L
    result = smooth_min(result, SDF_Sphere(query, vec3(0.5, -0.4, 0.5), 0.5), 0.3);
    // cheek R
    result = smooth_min(result, SDF_Sphere(query, vec3(-0.5, -0.4, 0.5), 0.5), 0.3);
    // chin
    result = smooth_min(result, SDF_Sphere(query, vec3(0.0, -0.85, 0.5), 0.35), 0.3);
    // nose
    result = smooth_min(result, SDF_Sphere(query / vec3(1.15,1,1), vec3(0, -0.2, 1.15), 0.35), 0.05);
    return result;
}

float SDF_Wahoo_Hat(vec3 query) {
    float result = SDF_Sphere(rotateX(query, 20) / vec3(1.1,0.5,1), vec3(0,1.65,0.4), 1.);
    result = smooth_min(result, SDF_Sphere((query - vec3(0,0.7,-0.95)) / vec3(2.5, 1.2, 1), vec3(0,0,0), 0.2), 0.3);
    result = smooth_min(result, SDF_Sphere(query / vec3(1.5,1,1), vec3(0, 1.3, 0.65), 0.5), 0.3);

    float brim = opOnion(SDF_Sphere(query / vec3(1.02, 1, 1), vec3(0, -0.15, 1.), 1.1), 0.02);

    brim = subtract(brim, SDF_Box(rotateX(query - vec3(0, -0.55, 0), 10), vec3(10, 1, 10)));

    result = min(result, brim);

    return result;
}


float SDF_Wahoo(vec3 query) {
    // Flesh-colored parts
    float result = SDF_Wahoo_Skin(query);
    // 'stache parts
    result = min(result, SDF_Stache(query));
    // hat
    result = min(result, SDF_Wahoo_Hat(query));

    return result;
}

// BSDF BSDF_Wahoo(vec3 query) {
//     // Head base
//     BSDF result = BSDF(query, normalize(query), pow(vec3(239, 181, 148) / 255., vec3(2.2)),
//                        0., 0.7, 1.);

//     result.nor = SDF_Normal(query);

//     float skin = SDF_Wahoo_Skin(query);
//     float stache = SDF_Stache(query);
//     float hat = SDF_Wahoo_Hat(query);

//     if(stache < skin && stache < hat) {
//         result.albedo = pow(vec3(68,30,16) / 255., vec3(2.2));
//     }
//     if(hat < skin && hat < stache) {
//         result.albedo = pow(vec3(186,45,41) / 255., vec3(2.2));
//     }

//     return result;
// }

float SDF_Cylinder(vec3 p, vec3 a, vec3 b, float r) {
    vec3 ba = b - a;
    vec3 pa = p - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h) - r;
}

float SDF_Capsule(vec3 p, vec3 a, vec3 b, float r) {
    vec3 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h) - r;
}

float SDF_Torus(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xy) - t.x, p.z);
    return length(q) - t.y;
}

float displacement(vec3 p) {
    return 0.05 * sin(10.0 * p.x + u_Time) * sin(10.0 * p.y + u_Time) * sin(10.0 * p.z + u_Time);
}


float opDisplace(float sdfValue, vec3 p) {
    float disp = displacement(p);
    return sdfValue + disp;
}

float CatHead(vec3 query) {
    return opDisplace(SDF_Sphere(query, vec3(0.0, 0.0, 0.0), 1.0), query);
}

float CatEars(vec3 query) {
    float leftEar = opDisplace(SDF_Capsule(query, vec3(-0.5, 0.8, 0.0), vec3(-0.2, 1.1, 0.0), 0.2), query);
    float rightEar = opDisplace(SDF_Capsule(query, vec3(0.5, 0.8, 0.0), vec3(0.2, 1.1, 0.0), 0.2), query);
    return smooth_min(leftEar, rightEar, 0.1);
}

float CatWhiskers(vec3 query) {
    float whiskerLeftTop = SDF_Cylinder(query, vec3(-0.2, 0.05, 0.9), vec3(-0.7, 0.1, 0.9), 0.02);
    float whiskerLeftMid = SDF_Cylinder(query, vec3(-0.2, 0.0, 0.9), vec3(-0.7, 0.0, 0.9), 0.02);
    float whiskerLeftBot = SDF_Cylinder(query, vec3(-0.2, -0.05, 0.9), vec3(-0.7, -0.1, 0.9), 0.02);
    float whiskersLeft = smooth_min(smooth_min(whiskerLeftTop, whiskerLeftMid, 0.05), whiskerLeftBot, 0.05);

    float whiskerRightTop = SDF_Cylinder(query, vec3(0.2, 0.05, 0.9), vec3(0.7, 0.1, 0.9), 0.02);
    float whiskerRightMid = SDF_Cylinder(query, vec3(0.2, 0.0, 0.9), vec3(0.7, 0.0, 0.9), 0.02);
    float whiskerRightBot = SDF_Cylinder(query, vec3(0.2, -0.05, 0.9), vec3(0.7, -0.1, 0.9), 0.02);
    float whiskersRight = smooth_min(smooth_min(whiskerRightTop, whiskerRightMid, 0.05), whiskerRightBot, 0.05);

    return smooth_min(whiskersLeft, whiskersRight, 0.1);
}

float CatCollar(vec3 query) {
    vec3 collarPosition = rotateX(query - vec3(0.0, -1.0, 0.0), 90.0);
    return opDisplace(SDF_Torus(collarPosition, vec2(0.5, 0.1)), collarPosition);
}

float CatEyes(vec3 query) {
    float leftEye = SDF_Sphere(query, vec3(-0.2, 0.3, 1.0), 0.1);  // Position and size of the left eye
    float rightEye = SDF_Sphere(query, vec3(0.2, 0.3, 1.0), 0.1);  // Position and size of the right eye
    return smooth_min(leftEye, rightEye, 0.1);
}

vec3 myrotateY(vec3 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec3(c * p.x + s * p.z, p.y, c * p.z - s * p.x);
}

vec3 myrotateX(vec3 p, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return vec3(p.x, c * p.y - s * p.z, s * p.y + c * p.z);
}

vec3 randomRotation(vec3 p, vec3 gridIndex) {
    float angleY = fract(sin(dot(gridIndex, vec3(12.9898, 78.233, 54.53))) * 43758.5453) * radians(50.0);
    float angleX = fract(cos(dot(gridIndex, vec3(93.9898, 67.345, 54.53))) * 43758.5453) * radians(50.0);
    p = myrotateY(p, angleY * sin(u_Time));  // Rotate over time
    p = myrotateX(p, angleX * cos(u_Time));  // Rotate over time
    return p;
}

float SDF_Cat(vec3 query, vec3 gridIndex) {
    query = randomRotation(query, gridIndex);
    float head = CatHead(query);
    float ears = CatEars(query);
    float whiskers = CatWhiskers(query);
    float collar = CatCollar(query);
    float nose = SDF_Sphere(query, vec3(0.0, 0.0, 1.02), 0.1);
    float eyes = CatEyes(query);

    float headAndEyes = subtract(head, eyes);
    float catHead = smooth_min(smooth_min(headAndEyes, ears, 0.2), nose, 0.1);
    float dingDongCat = smooth_min(smooth_min(catHead, collar, 0.2), whiskers, 0.1);

    return dingDongCat;
}

float calculateThinness(vec3 pos, vec3 nor, float delta, float k) {
    float ao = 0.0;
    for (int i = 1; i <= 5; ++i) {
        float sampleDistance = i * delta;
        float sdfResult = sceneSDF(pos + nor * sampleDistance);
        ao += (1.0 / pow(2.0, float(i))) * (sampleDistance - sdfResult);
    }
    ao *= k; // Apply the scaling factor
    float thinness = 1.0 - ao; // Convert AO to thinness
    thinness = clamp(thinness, 0.0, 1.0); // Ensure thinness is within [0,1]
    return thinness;
}

vec3 subsurfaceScattering(BSDF bsdf, vec3 viewVec) {
    float subsurfaceScale = 3.0;
    float subsurfaceGlow = 6.0;
    float subsurfaceDistortion = 0.2;

    vec3 invertedView = -viewVec;
    vec3 lightDir = normalize(invertedView); // Approximate the light direction using the view vector

    // Calculate the scattering direction
    vec3 scatterDir = lightDir + bsdf.nor * subsurfaceDistortion;

    // Calculate the amount of light that reaches the eye
    float lightReachingEye = pow(clamp(dot(viewVec, -scatterDir), 0.0, 1.0), subsurfaceGlow) * subsurfaceScale;

    // Fetch the diffuse irradiance
    vec3 irradiance = texture(u_DiffuseIrradianceMap, scatterDir).rgb;

    // Compute the final scattered light contribution
    float attenuation = max(0.0, dot(bsdf.nor, lightDir) + dot(viewVec, -lightDir)) * (1.0 - bsdf.metallic);
    float totalLight = attenuation * (lightReachingEye + 0.3) * bsdf.thinness; // 0.3 is assumed ambient component

    return bsdf.albedo * irradiance * totalLight;
}


BSDF BSDF_Cat(vec3 query, vec3 gridIndex) {
    query = randomRotation(query, gridIndex);
    // Albedo color varies with X axis
    float redComponent = fract(sin(dot(vec3(gridIndex.x), vec3(0.1)) * 10.0) * 0.5 + 0.5);
    float greenComponent = fract(sin(dot(vec3(gridIndex.x + 0.5), vec3(0.1)) * 10.0 + 2.0) * 0.5 + 0.5);
    float blueComponent = fract(sin(dot(vec3(gridIndex.x + 1.0), vec3(0.1)) * 10.0 + 4.0) * 0.5 + 0.5);
    vec3 albedo = vec3(redComponent, greenComponent, blueComponent);

    // Metallic varies with Z axis
    float metallic = fract(sin(dot(vec3(gridIndex.z), vec3(1.0)) * 12.345));

    // Roughness varies with Y axis
    float roughness = clamp(sin(dot(vec3(gridIndex.y), vec3(1.0)) * 0.987), 0.05, 0.95);

    vec3 normal = SDF_Normal(query);
    float ao = 1.0;
    float earDistance = CatEars(query);
    float whiskerDistance = CatWhiskers(query);
    float collarDistance = CatCollar(query);
    float noseDistance = opDisplace(SDF_Sphere(query, vec3(0.0, 0.0, 1.0), 0.1), query);
    float eyeDistance = CatEyes(query);

    if (earDistance < 0.1) {
        albedo = mix(albedo, pow(vec3(255, 255, 0) / 255., vec3(2.2)), smoothstep(0.1, 0.01, earDistance));
    }
    if (eyeDistance < 0.00001) {
        albedo = vec3(0.0, 0.0, 0.0);
    }
    if (whiskerDistance < 0.01) {
        albedo = vec3(0.0, 0.0, 0.0);
    }
    if (collarDistance < 0.05) {
        albedo = mix(albedo, pow(vec3(255, 105, 97) / 255., vec3(2.2)), smoothstep(0.1, 0.01, collarDistance));
    }
    if (noseDistance < 0.05) {
        albedo = mix(albedo, pow(vec3(255, 255, 0) / 255., vec3(2.2)), smoothstep(0.1, 0.01, noseDistance));
    }

    float thinness = calculateThinness(query, normal, 0.085, 2.0);
    return BSDF(query, normal, albedo, metallic, roughness, ao, thinness);
}

float opSmoothUnion(float d1, float d2, float k) {
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h);
}

float opRepetition(vec3 p, vec3 cellSize) {
    vec3 gridIndex = round(p / cellSize);
    vec3 basePos = cellSize * gridIndex;

    float timeOffset = u_Time + length(gridIndex) * 10;
    vec3 direction = normalize(vec3(sin(gridIndex.x + timeOffset), cos(gridIndex.y + timeOffset), sin(gridIndex.z + timeOffset)));
    float movementScale = 0.2 * cellSize.x;  // Control the extent of movement
    vec3 offset = direction * sin(u_Time + length(gridIndex)) * movementScale;

    vec3 localPos = p - basePos + offset;

    float d = SDF_Cat(localPos, gridIndex);

    float k = 0.1;
    vec3 neighborOffsets[6];
    neighborOffsets[0] = vec3(1, 0, 0);
    neighborOffsets[1] = vec3(-1, 0, 0);
    neighborOffsets[2] = vec3(0, 1, 0);
    neighborOffsets[3] = vec3(0, -1, 0);
    neighborOffsets[4] = vec3(0, 0, 1);
    neighborOffsets[5] = vec3(0, 0, -1);

    for(int i = 0; i < 6; i++) {
        vec3 neighborOffset = neighborOffsets[i] * cellSize;
        float neighborDist = SDF_Cat(localPos - neighborOffset, gridIndex);
        d = opSmoothUnion(d, neighborDist, k);
    }

    return d;
}


float sceneSDF(vec3 query) {
    vec3 globalMovement = vec3(
        sin(u_Time * 0.5 + query.x * 0.5),
        cos(u_Time * 0.5 + query.y * 0.5),
        sin(u_Time * 0.5 + query.z * 0.5)
    ) * 0.1;
    vec3 adjustedQuery = query + globalMovement;
    vec3 gridIndex = round(adjustedQuery / cellSize);
    return opRepetition(adjustedQuery, cellSize);
}

BSDF sceneBSDF(vec3 query) {
    vec3 globalMovement = vec3(
        sin(u_Time * 0.5 + query.x * 0.5),
        cos(u_Time * 0.5 + query.y * 0.5),
        sin(u_Time * 0.5 + query.z * 0.5)
    ) * 0.1;

    vec3 adjustedQuery = query + globalMovement;
    vec3 gridIndex = round(adjustedQuery / cellSize);
    vec3 basePos = cellSize * gridIndex;

    float timeOffset = u_Time + length(gridIndex) * 10;
    vec3 direction = normalize(vec3(sin(gridIndex.x + timeOffset), cos(gridIndex.y + timeOffset), sin(gridIndex.z + timeOffset)));
    float movementScale = 0.2 * cellSize.x;
    vec3 offset = direction * sin(u_Time + length(gridIndex)) * movementScale;

    vec3 localPos = adjustedQuery - basePos + offset;

    vec3 newGridIndex = round((localPos + basePos) / cellSize);

    return BSDF_Cat(localPos, newGridIndex);
}




// TODO add any helper functions you need here

vec3 metallic_plastic_LTE(BSDF bsdf, vec3 wo) {
    vec3 N = bsdf.nor;
    vec3 albedo = bsdf.albedo;
    float metallic = bsdf.metallic;
    float roughness = bsdf.roughness;
    float ambientOcclusion = bsdf.ao;

    // TODO
    // Fresnel term using Schlick's approximation
    vec3 F0 = mix(vec3(0.04), albedo, metallic); // Base reflectivity
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(N, wo), 0.0), 5.0);

    // Sample the BRDF lookup texture for the D and G terms
    vec2 brdfSample = texture(u_BRDFLookupTexture, vec2(max(dot(N, wo), 0.0), roughness)).rg;
    vec3 specular = vec3(0.0);

    // Calculate the reflection vector for glossy specular
    vec3 R = reflect(-wo, N); // Reflect wo about N to get wi
    const float maxMipLevels = 4.0;

    // Sample the glossy irradiance map using wi and determine the mip level based on roughness
    vec3 glossyIrradiance = textureLod(u_GlossyIrradianceMap, R, roughness * maxMipLevels).rgb;
    specular += glossyIrradiance * (F * brdfSample.x + brdfSample.y);

    // Compute kD as (1.0 - kS) and multiply with the diffuse component
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(u_DiffuseIrradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;

    vec3 color = (kD * diffuse + specular) * ambientOcclusion;
    return color;
}


#define FOVY 45 * PI / 180.f
Ray rayCast() {
    vec2 ndc = fs_UV;
    ndc = ndc * 2.f - vec2(1.f);

    float aspect = u_ScreenDims.x / u_ScreenDims.y;
    vec3 ref = u_CamPos + u_Forward;
    vec3 V = u_Up * tan(FOVY * 0.5);
    vec3 H = u_Right * tan(FOVY * 0.5) * aspect;
    vec3 p = ref + H * ndc.x + V * ndc.y;

    return Ray(u_CamPos, normalize(p - u_CamPos));
}

#define MAX_ITERATIONS 128
#define MAX_DISTANCE 150.0
#define EPSILON 0.0001
MarchResult raymarch(Ray ray) {
    float t = 0.0;
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        vec3 pos = ray.origin + t * ray.direction;
        float dist = sceneSDF(pos);
        if (dist < EPSILON) {
            return MarchResult(t, 1, sceneBSDF(pos));
        }
        t += dist;
        if (t >= MAX_DISTANCE) break;
    }
    return MarchResult(-1, 0, BSDF(vec3(0.), vec3(0.), vec3(0.), 0., 0., 0., 0.));
}

void main()
{
    Ray ray = rayCast();
    MarchResult result = raymarch(ray);
    BSDF bsdf = result.bsdf;
    vec3 pos = ray.origin + result.t * ray.direction;
    vec3 viewVec = normalize(u_CamPos - pos);

    vec3 color = metallic_plastic_LTE(bsdf, -ray.direction);
    vec3 subsurface = subsurfaceScattering(bsdf, viewVec);
    color += subsurface;
    // Reinhard operator to reduce HDR values from magnitude of 100s back to [0, 1]
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    out_Col = vec4(color, result.hitSomething > 0 ? 1. : 0.);
}

 
