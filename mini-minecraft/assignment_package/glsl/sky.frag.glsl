#version 330 core

uniform mat4 u_ViewProj;    // We're actually passing the inverse of the viewproj
                            // from our CPU, but it's named u_ViewProj so we don't
                            // have to bother rewriting our ShaderProgram class

uniform ivec2 u_Dimensions; // Screen dimensions

uniform vec3 u_lightDir;

uniform vec3 u_Eye; // Camera pos

uniform float u_Time;

layout (location = 0) out vec4 gb_Sky;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// Sunset palette
const vec3 sunset[5] = vec3[](vec3(255, 229, 119) / 255.0,
                               vec3(254, 192, 81) / 255.0,
                               vec3(255, 137, 103) / 255.0,
                               vec3(253, 96, 81) / 255.0,
                               vec3(57, 32, 51) / 255.0);
// Dusk palette
const vec3 dusk[5] = vec3[](vec3(144, 96, 144) / 255.0,
                            vec3(96, 72, 120) / 255.0,
                            vec3(72, 48, 120) / 255.0,
                            vec3(48, 24, 96) / 255.0,
                            vec3(0, 24, 72) / 255.0);
//noon palette
const vec3 noon[5] = vec3[](
    vec3(173, 216, 255) / 255.0,
    vec3(135, 206, 250) / 255.0,
    vec3(74, 145, 255) / 255.0,
    vec3(28, 127, 238) / 255.0,
    vec3(16, 78, 139) / 255.0);

//sun color
const vec3 sunColor = vec3(255, 255, 190) / 255.0;
const vec3 cloudColor = sunset[3];

vec2 sphereToUV(vec3 p) {
    float phi = atan(p.z, p.x);
    if(phi < 0) {
        phi += TWO_PI;
    }
    float theta = acos(p.y);
    return vec2(1 - phi / TWO_PI, 1 - theta / PI);
}

vec3 getColorFromPalette(vec2 uv, const vec3 palette[5]) {
    if (uv.y < 0.5) return palette[0];
    else if (uv.y < 0.55) return mix(palette[0], palette[1], (uv.y - 0.5) / 0.05);
    else if (uv.y < 0.6) return mix(palette[1], palette[2], (uv.y - 0.55) / 0.05);
    else if (uv.y < 0.65) return mix(palette[2], palette[3], (uv.y - 0.6) / 0.05);
    else if (uv.y < 0.75) return mix(palette[3], palette[4], (uv.y - 0.65) / 0.1);
    return palette[4];
}

vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 random3( vec3 p ) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                          dot(p,vec3(269.5, 183.3, 765.54)),
                          dot(p, vec3(420.69, 631.2,109.21))))
                 *43758.5453);
}


vec3 rotateX(vec3 p, float a) {
    return vec3(p.x, cos(a) * p.y + -sin(a) *p.z, sin(a) * p.y +cos(a) * p.z);
}

float WorleyNoise3D(vec3 p)
{
    // Tile the space
    vec3 pointInt = floor(p);
    vec3 pointFract = fract(p);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int z = -1; z <= 1; z++)
    {
        for(int y = -1; y <= 1; y++)
        {
            for(int x = -1; x <= 1; x++)
            {
                vec3 neighbor = vec3(float(x), float(y), float(z));

                // Random point inside current neighboring cell
                vec3 point = random3(pointInt + neighbor);

                // Animate the point
                point = 0.5 + 0.5 * sin(u_Time * 0.01 + 6.2831 * point); // 0 to 1 range

                // Compute the distance b/t the point and the fragment
                // Store the min dist thus far
                vec3 diff = neighbor + point - pointFract;
                float dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }
    return minDist;
}

float WorleyNoise(vec2 uv)
{
    // Tile the space
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for(int y = -1; y <= 1; y++)
    {
        for(int x = -1; x <= 1; x++)
        {
            vec2 neighbor = vec2(float(x), float(y));

            // Random point inside current neighboring cell
            vec2 point = random2(uvInt + neighbor);

            // Animate the point
            point = 0.5 + 0.5 * sin(u_Time * 0.01 + 6.2831 * point); // 0 to 1 range

            // Compute the distance b/t the point and the fragment
            // Store the min dist thus far
            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

float worleyFBM(vec3 uv) {
    float sum = 0;
    float freq = 4;
    float amp = 0.5;
    for(int i = 0; i < 8; i++) {
        sum += WorleyNoise3D(uv * freq) * amp;
        freq *= 2;
        amp *= 0.5;
    }
    return sum;
}

//#define RAY_AS_COLOR
//#define SPHERE_UV_AS_COLOR
#define WORLEY_OFFSET

void main()
{
    vec2 ndc = (gl_FragCoord.xy / vec2(u_Dimensions)) * 2.0 - 1.0; // -1 to 1 NDC

    vec4 p = vec4(ndc.xy, 1, 1); // Pixel at the far clip plane
    p *= 1000.0; // Times far clip plane value
    p = /*Inverse of*/ u_ViewProj * p; // Convert from unhomogenized screen to world

    vec3 rayDir = normalize(p.xyz - u_Eye);

    //make an illusion that the quad is spherical shape
    vec2 uv = sphereToUV(-rayDir);


    //make offset for noise in the sky
    vec2 offset = vec2(0.0);
#ifdef WORLEY_OFFSET
    // Get a noise value in the range [-1, 1]
    // by using Worley noise as the noise basis of FBM
    offset = vec2(worleyFBM(rayDir));
    offset * 2;
    offset += vec2(2.5);
#endif

    // gradient from the bottom to the top
    vec3 sunsetCol = getColorFromPalette(uv + offset * 0.1, sunset);
    vec3 duskCol = getColorFromPalette(uv + offset * 0.1, dusk);
    vec3 noonCol = getColorFromPalette(uv + offset * 0.1, noon);

    vec3 x = vec3(1, 0, 0);
    vec3 y = vec3(0, 1, 0);
    vec3 z = vec3(0, 0, 1);

    // Add a glowing sun in the sky
    vec3 sunDir = normalize(rotateX(normalize(vec3(0, 0, -1.0)), u_Time * 0.05));

    float sunSize = 30.0;
    float angle = (acos(dot(rayDir, sunDir)) * 360.0 / PI);
    float dirAngle = dot(sunDir, y);

#define SUNSET_THRESHOLD 0.0
#define NOON_THRESHOLD 0.5
#define DUSK_THRESHOLD -0.75

    if (dirAngle > NOON_THRESHOLD) {
        if(angle < sunSize) {
            if(angle < 7.5) {
                gb_Sky = vec4(sunColor, 1);
            }
            // Corona of sun, mix with the current sky color
            else {
                gb_Sky = vec4(mix(sunColor, noonCol, (angle - 7.5) / 22.5), 1);
            }
        } else {
            gb_Sky = vec4(noonCol, 1);
        }

    //between sunset and noon
    } else if (dirAngle > SUNSET_THRESHOLD) {
        if(angle < sunSize) {
            // Full center of sun
            if(angle < 7.5) {
                gb_Sky = vec4(sunColor, 1);
            }
            // Corona of sun, mix with the current sky color
            else {
                float t = (dirAngle - SUNSET_THRESHOLD) / (NOON_THRESHOLD - SUNSET_THRESHOLD);
                gb_Sky = vec4(mix(sunColor, mix(sunsetCol, noonCol, t), (angle - 7.5) / 22.5), 1);
            }
            //if not within sun angle, we set the color to noon & sunset
        } else {
            float t = (dirAngle - SUNSET_THRESHOLD) / (NOON_THRESHOLD - SUNSET_THRESHOLD);
            gb_Sky = vec4(mix(sunsetCol, noonCol, t), 1);
        }

    //between dusk and sunset
    } else if (dirAngle > DUSK_THRESHOLD) {
        if(angle < sunSize) {
            // Full center of sun
            if(angle < 7.5) {
                gb_Sky = vec4(sunColor, 1);
            }
            // Corona of sun, mix with the current sky color
            else {
                float t = (dirAngle - SUNSET_THRESHOLD) / (DUSK_THRESHOLD - SUNSET_THRESHOLD);
                gb_Sky = vec4(mix(sunColor, mix(sunsetCol, duskCol, t), (angle - 7.5) / 22.5), 1);
            }
        } else {
            float t = (dirAngle - SUNSET_THRESHOLD) / (DUSK_THRESHOLD - SUNSET_THRESHOLD);
            gb_Sky = vec4(mix(sunsetCol, duskCol, t), 1);
        }

    } else {
        float t = (dirAngle - SUNSET_THRESHOLD) / (DUSK_THRESHOLD - SUNSET_THRESHOLD);
        gb_Sky = vec4(mix(sunsetCol, duskCol, t), 1);
    }
}
