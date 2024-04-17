#version 330 core

uniform sampler2D u_TexPositionWorld;
uniform sampler2D u_TexNormal;
uniform sampler2D u_TexAlbedo;
uniform sampler2D u_TexMetalRoughMask;
uniform sampler2D u_TexPBR;

uniform vec3 u_CamPos;
uniform vec3 u_CamForward;
uniform mat4 u_View;
uniform mat4 u_Proj;

in vec2 fs_UV;

out vec4 out_Col;


layout (location = 0) out vec4 gb_Reflection;

float maxDistance = 15.f;
int stepSize = 200;
float threshold = 1.f;

vec2 uvWorld2Screen(vec4 worldPos) {
    vec4 uv = u_Proj * u_View * worldPos;
    uv.xyz /= uv.w;
    return uv.xy * 0.5 + 0.5;
}

float depthCamera2World(vec4 worldPos) {
    return dot(worldPos.xyz - u_CamPos, u_CamForward);
}


bool rayMarchFirstPass(vec4 startMarchWorld, vec4 endMarchWorld, out float currentStepFraction, out float lastMissStepFraction) {
    lastMissStepFraction = 0.0;
    bool isIntersect = false;

    for (int i = 1; i <= stepSize; i++) {
        currentStepFraction = float(i) / stepSize;
        vec4 marchPointWorld = mix(startMarchWorld, endMarchWorld, currentStepFraction);
        vec2 uv = uvWorld2Screen(marchPointWorld);

        if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
            break;
        }

        vec4 worldPosSampled = texture(u_TexPositionWorld, uv);
        if (worldPosSampled.a == 0.0) {
            continue;
        }
        worldPosSampled.a = 1.0;

        float depth = depthCamera2World(marchPointWorld) - depthCamera2World(worldPosSampled);
        if (depth > 0 && depth < threshold) {
            isIntersect = true;
            break;
        } else {
            lastMissStepFraction = currentStepFraction;
        }
    }

    return isIntersect;
}

bool rayMarchBinarySearch(vec4 startMarchWorld, vec4 endMarchWorld, float lastMissStepFraction, float currentStepFraction, out vec2 uv, out vec4 marchPointWorld, out float depth) {
    float low = lastMissStepFraction;
    float high = currentStepFraction;
    bool isIntersect = false;

    for (int i = 0; i < stepSize; i++) {
        currentStepFraction = (low + high) / 2.0;
        marchPointWorld = mix(startMarchWorld, endMarchWorld, currentStepFraction);
        uv = uvWorld2Screen(marchPointWorld);

        vec4 worldPosSampled = texture(u_TexPositionWorld, uv);
        if (worldPosSampled.a == 0.0) {
            low = currentStepFraction;
            continue;
        }
        worldPosSampled.a = 1.0;

        depth = depthCamera2World(marchPointWorld) - depthCamera2World(worldPosSampled);
        if (depth > 0 && depth < threshold) {
            high = currentStepFraction;  // Closer to an intersection
            isIntersect = true;
        } else {
            low = currentStepFraction;  // Move away from this point
        }
    }

    return isIntersect;
}

void main() {
    vec3 worldPos = texture(u_TexPositionWorld, fs_UV).xyz;
    vec3 wo = normalize(worldPos - u_CamPos);
    vec3 normal = normalize(texture(u_TexNormal, fs_UV).xyz);
    vec3 wi = normalize(reflect(wo, normal));

    if (dot(-wo, normal) < 0) {
        gb_Reflection = vec4(0.0);
        return;
    }

    vec4 startMarchWorld = vec4(worldPos + normal * threshold * 0.1, 1.0);
    vec4 endMarchWorld = vec4(worldPos + (wi * maxDistance), 1.0);

    float currentStepFraction, lastMissStepFraction;
    bool hit0 = rayMarchFirstPass(startMarchWorld, endMarchWorld, currentStepFraction, lastMissStepFraction);

    if (!hit0) {
        gb_Reflection = vec4(0.0);
        return;
    }

    vec2 uv;
    vec4 marchPointWorld;
    float depth;
    bool hit1 = rayMarchBinarySearch(startMarchWorld, endMarchWorld, lastMissStepFraction, currentStepFraction, uv, marchPointWorld, depth);

    float visibility = float(hit1)
        * texture(u_TexMetalRoughMask, uv).b
        * (1 - max(dot(-wo, wi), 0))
        * (1 - clamp(depth / threshold, 0, 1))
        * (1 - clamp(distance(marchPointWorld.xyz, worldPos.xyz) / maxDistance, 0, 1))
        * (1 - smoothstep(0.95f, 1.f, uv.y))
        * (smoothstep(0.f, 0.05f, uv.y))
        * (smoothstep(0.f, 0.05f, uv.x))
        * (1 - smoothstep(0.95f, 1.f, uv.x));
    visibility = clamp(visibility, 0, 1);

    gb_Reflection = vec4(texture(u_TexPBR, uv).rgb, visibility);
}
