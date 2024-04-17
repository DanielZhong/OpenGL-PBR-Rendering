#version 330 core

uniform sampler2D u_TexPositionWorld;
uniform sampler2D u_TexNormal;
uniform sampler2D u_TexAlbedo;
uniform sampler2D u_TexMetalRoughMask;
uniform sampler2D u_TexPBR;

// Image-based lighting
uniform samplerCube u_DiffuseIrradianceMap;
uniform samplerCube u_GlossyIrradianceMap;
uniform sampler2D u_BRDFLookupTexture;

uniform vec3 u_CamPos;
uniform vec3 u_CamForward;
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform mat4 u_ProjInv;

in vec2 fs_UV;

out vec4 out_Col;

const float PI = 3.14159f;

// Smith's Schlick-GGX approximation
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Trowbridge-Reitz GGX microfacet distribution
// An approximation of the Trowbridge-Reitz D() function from PBRT
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// F0 is surface reflection at zero incidence (looking head on)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    float ct = clamp(1.0 - cosTheta, 0.0, 1.0);
    return F0 + (1.0 - F0) * ((ct * ct) * (ct * ct) * ct);
}

// Same as above, but accounts for surface roughness
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 computeLTE(vec3 pos, vec3 N, vec3 albedo, float metallic, float roughness) {
    vec3 V = normalize(u_CamPos - pos);
    vec3 R = reflect(-V, N);

    vec3 Lo = vec3(0.f);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= (1.0 - metallic);

    // Sample our diffuse illumination & combine it with our albedo
    vec3 diffuseIrradiance = texture(u_DiffuseIrradianceMap, N).rgb;
    vec3 diffuse = diffuseIrradiance * albedo;

    // Sample the glossy irradiance map & the BRDF lookup texture
    // Combine these values via the split-sum approximation
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 glossyIrradiance = textureLod(u_GlossyIrradianceMap, R,
                                       roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(u_BRDFLookupTexture,
                        vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = glossyIrradiance * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular);
    vec3 color = ambient + Lo;

    return color;
}

vec4 computeLTE(vec3 pos, vec3 N,
                vec3 albedo, float metallic, float roughness,
                vec3 wo, vec4 Li) {
    vec3 V = wo;
    vec3 R = reflect(-wo, N);

    vec3 Lo = vec3(0.f);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= (1.0 - metallic);

    // Sample our diffuse illumination & combine it with our albedo
    // TODO: Have to make another post-process pass
    // that takes in the blurred reflection mip-mapped texture
    // in order to handle non-mirror surfaces
    vec3 diffuseIrradiance = vec3(0.);//texture(u_DiffuseIrradianceMap, N).rgb;
    vec3 diffuse = diffuseIrradiance * albedo;

    // Sample the glossy irradiance map & the BRDF lookup texture
    // Combine these values via the split-sum approximation
    const float MAX_REFLECTION_LOD = 4.0;

    // TODO: Have to make another post-process pass
    // that takes in the blurred reflection mip-mapped texture
    // in order to handle non-mirror surfaces
    vec3 glossyIrradiance = Li.rgb;
//            textureLod(u_GlossyIrradianceMap, R,
//                                       roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(u_BRDFLookupTexture,
                        vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = glossyIrradiance * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular);
    vec3 color = ambient + Lo;

    return vec4(color, Li.a);
}

vec3 screenSpaceRefl(vec3 pos, vec3 nor);
vec3 SSR_copied(vec4 positionFrom, vec3 normal);
vec4 SSR_WorldSpace(out vec3 wi);

void main() {
    vec3 mrm = texelFetch(u_TexMetalRoughMask, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 pos = texelFetch(u_TexPositionWorld, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 nor = texelFetch(u_TexNormal, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 alb = pow(texelFetch(u_TexAlbedo, ivec2(gl_FragCoord.xy), 0).rgb, vec3(2.2));

    vec4 no_refl_color = vec4(computeLTE(pos, nor, alb, mrm.r, mrm.g), mrm.b);

    vec3 wi;
    vec4 refl_color = SSR_WorldSpace(wi);
    vec4 refl_color2 = computeLTE(pos, nor,
                            alb, mrm.r, mrm.g,
                            normalize(u_CamPos - pos), refl_color);

    vec4 final_color = vec4(
                            mix(no_refl_color.rgb,
                                refl_color2.rgb,
                                refl_color2.a),
                            no_refl_color.a
                        );

    final_color.rgb = final_color.rgb / (final_color.rgb + vec3(1.0));
    final_color.rgb = pow(final_color.rgb, vec3(1.0/2.2));

    out_Col = final_color;
}

float depthFromWorld(vec4 worldPos) {
    return dot(worldPos.xyz - u_CamPos, u_CamForward);
}

vec2 uvFromWorldPos(vec4 worldPos) {
    vec4 uv = u_Proj * u_View * worldPos;
    uv.xyz /= uv.w;
    return uv.xy * 0.5 + 0.5;
}

vec4 SSR_WorldSpace(out vec3 wi) {
    float maxDistance = 10.f;
    int steps1 = 200;
    int steps2 = 20;
    float thickness1 = 0.7f;
    float thickness2 = 0.25f;

    vec3 worldPos = texture(u_TexPositionWorld, fs_UV).xyz;
    vec3 wo = normalize(worldPos - u_CamPos);
    vec3 normal = normalize(texture(u_TexNormal, fs_UV).xyz);
    wi = normalize(reflect(wo, normal));

//    return vec4(0.5 * (wi + vec3(1.)), 1.);

    // No refl if viewing back face
    if (dot(-wo, normal) < 0) {
        return vec4(0.);
    }

    vec4 startMarchWorld = vec4(worldPos.xyz + normal * thickness1 * 0.1, 1);
    vec4 endMarchWorld = vec4(worldPos.xyz + (wi * maxDistance), 1);

    float t_last_miss = 0; // last point at which the ray missed geometry
    float t_curr = 0; // current interpolation point

    int hit0 = 0; // 1 if there was an intersection in the first pass
    int hit1 = 0; // 1 if there was an intersection in the second pass

    vec4 marchPointWorld;
    vec2 uv;
    float depth;

    // first pass
    for (int i = 1; i <= steps1; i++) {
        t_curr = float(i) / steps1;
        marchPointWorld = mix(startMarchWorld, endMarchWorld, t_curr);
        uv = uvFromWorldPos(marchPointWorld);

        if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
            break;
        }

        vec4 worldPosSampled = texture(u_TexPositionWorld, uv);
        // Skip over pixels with no geometry
        if(worldPosSampled.a == 0.) {
            continue;
        }
        worldPosSampled.a = 1.;

        depth = depthFromWorld(marchPointWorld) -
                depthFromWorld(worldPosSampled);
        if (depth > 0 && depth < thickness1) {
            hit0 = 1;
//            return vec4(uv, 0, 1);
//            return texture(u_TexNormal, uv);
//            return vec4(0, t_curr, 0, 1.);
            break;
        } else {
            t_last_miss = t_curr;
        }
    }
//    return vec4(depth, depth, depth, 1.);

    // REMOVE THIS
//    hit1 = hit0;

    // second pass
    t_curr = t_last_miss + ((t_curr - t_last_miss) / 2);

//    steps2 *= 0;
    steps2 *= hit0;
    // second pass is skipped if first pass did not hit anything
    for (int i = 0; i < steps2; i++) {
        marchPointWorld = mix(startMarchWorld, endMarchWorld, t_curr);
        uv = uvFromWorldPos(marchPointWorld);

        vec4 worldPosSampled = texture(u_TexPositionWorld, uv);
        // Skip over pixels with no geometry
        if(worldPosSampled.a == 0.) {
            continue;
        }
        worldPosSampled.a = 1.;
        depth = depthFromWorld(marchPointWorld) - depthFromWorld(worldPosSampled);
        if (depth > 0 && depth < thickness2) {
            hit1 = 1;
            t_curr = t_last_miss + ((t_curr - t_last_miss) / 2);
        } else {
            float temp = t_curr;
            t_curr = t_curr + ((t_curr - t_last_miss) / 2);
            t_last_miss = temp;
        }
    }

//    vec4 colorAtHit = texture(u_ColorTexture, uv);
    float visibility = hit1
            // Alpha of obj hit
            * texture(u_TexMetalRoughMask, uv).b//colorAtHit.a
            // Fade out the more retro-reflective the hit is
            * (1 - max(dot(-wo, wi), 0))
            // Reduce contrib. of hits that are further from the surface
            * (1 - clamp(depth / thickness2, 0, 1))
            // Fade out as we approach max dist to avoid abruptly cutting
            // off reflection
            * (1 - clamp(distance(marchPointWorld.xyz, worldPos.xyz) / maxDistance, 0, 1))
            // Fade out when UVs are near out of bounds
            * (1 - smoothstep(0.95f, 1.f, uv.y))
            * (smoothstep(0.f, 0.05f, uv.y))
            * (smoothstep(0.f, 0.05f, uv.x))
            * (1 - smoothstep(0.95f, 1.f, uv.x))
            ;
    visibility = clamp(visibility, 0, 1);// * specular.r;

//    visibility = hit1 * (1 - max(dot(-wo, wi), 0));

    vec3 obj_color = texture(u_TexPBR, uv).rgb;

    vec3 camSpaceHit = vec3(u_View * vec4(marchPointWorld.xyz, 1.));
    camSpaceHit = 0.5 * (camSpaceHit + vec3(1.));

    return vec4(obj_color, visibility);//mix(color, colorAtHit, visibility);
}

// OLD VERSIONS VVVVVVVVVVVVV
#if 0
vec3 screenSpaceRefl(vec3 pos, vec3 nor) {
    float maxDistance = 10;
    float resolution  = 1.0;
    int   steps       = 10;
    float thickness   = 1.0;

    vec2 gBufferDimensions = textureSize(u_TexPositionCamera, 0).xy;

    // Must convert normal to camera space since
    // all other operations & data are in camera space
    nor = vec4(u_View * vec4(nor, 0.)).xyz;

    vec3 wo = normalize(pos); // vector from cam to obj in camera space
    vec3 wi = normalize(reflect(wo, nor));

    // Endpoints of ray march line segment
    vec3 start_cam_space = pos;
    vec3 end_cam_space = pos + wi * maxDistance;

    // Compute pixel-space positions of start and end points
    vec4 start_frag = vec4(start_cam_space, 1.);
    // cam -> unhomogenized screen
    start_frag = u_Proj * start_frag;
    // unhomogenized screen -> screen via persp divide
    start_frag.xyz /= start_frag.w;
    // NDC -> UV coords
    start_frag.xy = (start_frag.xy + vec2(1.)) * 0.5;
    // UV coords -> texel coords
    start_frag.xy *= gBufferDimensions;

    vec4 end_frag = vec4(end_cam_space, 1.);
    end_frag = u_Proj * end_frag;
    end_frag.xyz /= end_frag.w;
    end_frag.xy = (end_frag.xy + vec2(1.)) * 0.5;
    end_frag.xy *= gBufferDimensions;

    // Begin raymarching in pixel space
    vec3 uv = vec3(0.); // XY is screen coords of reflected geom, Z is alpha of reflection
    vec2 frag = start_frag.xy;
    uv.xy = frag / gBufferDimensions;
    // Find slope of line in pixel space
    float dx = end_frag.x - start_frag.x;
    float dy = end_frag.y - start_frag.y;
    // Determine if the line is more horizontal or vertical.
    // This lets us use the larger slope as a t value for iterating
    // along line.
    float x_is_larger = abs(dx) >= abs(dy) ? 1 : 0;
    // delta is the larger of dx and dy. It is
    // attenuated by resolution; the smaller resolution,
    // the more pixels are skipped.
    float delta = mix(abs(dy), abs(dx), x_is_larger)
            * clamp(resolution, 0, 1);

    // Assuming resolution is 1, one of dx/dy will be
    // set to 1, and the other will be < 1. This iterates along
    // the ray one pixel at a time along its slope.
    vec2 ray_step = vec2(dx, dy) / max(delta, 0.001);

    float search0 = 0; // Remembers last pos on ray that missed geom
    float search1 = 0; // [0, 1]. 0 = @ start frag. 1 = @ end frag
    int hit0 = 0; // isect in first pass
    int hit1 = 0; // isect in second pass

    float cam_space_depth = start_cam_space.z;
    float depth = thickness;

    float i = 0;
    vec4 gBuffer_pos_at_march_point;
    for (i = 0; i < int(delta); ++i) {
        frag += ray_step;
        uv.xy = frag / gBufferDimensions;
        if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
            break;
        }
        gBuffer_pos_at_march_point = texture(u_TexPositionCamera, uv.xy);

        // Calculate the % along our line that we have traveled
        search1 = mix(
                    (frag.y - start_frag.y) / dy,
                    (frag.x - start_frag.x) / dx,
                    x_is_larger
                    );
        // Interpolate b/t cam-space depths at start and
        // end points of 3D ray based on % along 2D ray.
        // Do so using perspective-correct interpolation;
        // this is why it's not just mix(start.z, end.z, search1)
        cam_space_depth = (start_cam_space.z * end_cam_space.z)
                / mix(end_cam_space.z, start_cam_space.z, search1);

        // Now we compare the Z of our 3D ray point with
        // the Z of the G-buffer sampled at our 2D ray point
        depth = cam_space_depth - gBuffer_pos_at_march_point.z;

        if(depth > 0 && depth < thickness) {
            hit0 = 1;
            break;
        }
        else {
            search0 = search1;
        }
    }


    // Halfway between the position of the last miss and the
    // position of the last hit
    search1 = search0 + ((search1 - search0) / 2.0);

    // Will cause the second pass to be skipped if the first
    // pass hit nothing
    steps *= hit0;
#if 1
    // Second, refining raymarch. Effectively
    // performs a binary search for the most precise
    // hit possible on the ray
    for (i = 0; i < steps; ++i) {
        frag = mix(start_frag.xy, end_frag.xy, search1);
        uv.xy = frag / gBufferDimensions;
        if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
            break;
        }
        gBuffer_pos_at_march_point = texture(u_TexPositionCamera, uv.xy);

        cam_space_depth = (start_cam_space.z * end_cam_space.z)
                / mix(end_cam_space.z, start_cam_space.z, search1);
        depth = cam_space_depth - gBuffer_pos_at_march_point.z;

        // If the Z value is within our hit threshold, this is a "hit"
        // Binary search by setting search1 to
        // halfway between this HIT and the last known MISS position.
        if(depth > 0 && depth < thickness) {
            hit1 = 1;
            search1 = search0 + ((search1 - search0) / 2);
        }
        // If Z is outside the thickness range, this is a "miss"
        // Binary search by setting search1 to the midpoint between
        // this MISS and the last known HIT.
        else {
            float temp = search1;
            search1 = search1 + ((search1 - search0) / 2);
            search0 = temp;
        }
    }
#endif
    // We're done with the refining search.
    float visibility =
            // 0 or 1
            hit1 * // should be hit1, just debugging with 0
            // 0 if there is no geom at the hit point
            gBuffer_pos_at_march_point.w *
            // Fade out the reflection the more it is retro-reflective,
            // as the more wi faces the camera the likelier it is to
            // have actually hit the (invisible) backside of something
            (1 - max(dot(-wo, wi), 0)) *
            // The less precise our raymarch search is, the more faded its
            // color should be
            (1 - clamp(depth / thickness, 0, 1)) *
            // Fade out the reflection the farther the reflected object
            // is along the ray, preventing the reflections from abruptly
            // stopping past maxDistance
            (1 - clamp(length(gBuffer_pos_at_march_point.xyz - pos) / maxDistance, 0, 1)) *
            // If reflected point is outside frustum, no reflection
            (uv.x < 0 || uv.x > 1 ? 0 : 1) *
            (uv.y < 0 || uv.y > 1 ? 0 : 1);

    visibility = clamp(visibility, 0, 1);
    visibility = hit1 * gBuffer_pos_at_march_point.w; // deleteme
    // Store visibility in the last component of UV
    uv.b = visibility;
    return uv;
}

vec3 SSR_copied(vec4 positionFrom, vec3 normal) {
    float maxDistance = 8;
    float resolution  = 1;
    int   steps       = 10;
    float thickness1   = 0.01;
    float thickness2   = 0.3;

    vec2 texSize  = textureSize(u_TexPositionCamera, 0).xy;
    vec2 texCoord = gl_FragCoord.xy / texSize;

    vec4 uv = vec4(0.0);

//    vec4 positionFrom = texture(u_TexPositionCamera, texCoord);
//    vec4 mask         = texture(maskTexture,     texCoord);

    if (  positionFrom.w <= 0.0
//          || enabled.x      != 1.0
//          || mask.r         <= 0.0
          ) { return uv.rgb; }

    vec3 unitPositionFrom = normalize(positionFrom.xyz);
    normal = vec3(u_View * vec4(normal, 0.));
    vec3 pivot            = normalize(reflect(unitPositionFrom, normal));

    vec4 positionTo = positionFrom;

    vec4 startView = vec4(positionFrom.xyz + (pivot * (thickness1 + 0.001)) + (normal * 0.1), 1.0);
    vec4 endView   = vec4(positionFrom.xyz + (pivot * maxDistance), 1.0);

    vec4 startFrag      = startView;
    startFrag      = u_Proj * startFrag;
    startFrag.xyz /= startFrag.w;
    startFrag.x   = startFrag.x * 0.5 + 0.5;
    startFrag.y   = startFrag.y * 0.5 + 0.5;
    startFrag.xy  *= texSize;
    startFrag.z = startView.z; // possibly?

    vec4 endFrag      = endView;
    endFrag      = u_Proj * endFrag;
    endFrag.xyz /= endFrag.w;
    endFrag.x   = endFrag.x * 0.5 + 0.5;
    endFrag.y   = endFrag.y * 0.5 + 0.5;
    endFrag.xy  *= texSize;
    endFrag.z = endView.z; // possibly?

    vec2 frag  = startFrag.xy;
    uv.xy = frag / texSize;

    float deltaX    = endFrag.x - startFrag.x;
    float deltaY    = endFrag.y - startFrag.y;
    float useX      = abs(deltaX) >= abs(deltaY) ? 1.0 : 0.0;
    float delta     = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0.0, 1.0);
    vec2  increment = vec2(deltaX, deltaY) / max(delta, 0.001);

    float search0 = 0;
    float search1 = 0;

    int hit0 = 0;
    int hit1 = 0;

    float viewDistance = startView.y;
    float depth        = thickness1;

    float i = 0;

    for (i = 0; i < int(delta); ++i) {
        frag      += increment;
        uv.xy      = frag / texSize;
        if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
            break;
        }
        positionTo = texture(u_TexPositionCamera, uv.xy);

        search1 =
                mix
                ( (frag.y - startFrag.y) / deltaY
                  , (frag.x - startFrag.x) / deltaX
                  , useX
                  );

        search1 = clamp(search1, 0.0, 1.0);

        viewDistance = (startView.z * endView.z)
                       / mix(endView.z, startView.z, search1);
        // My version of computing Z
        // q is search1
//        viewDistance = 1.f / mix(1.f / startView.z, 1.f / endView.z, search1);

        depth        = viewDistance - positionTo.z;
//        depth        = positionTo.z - viewDistance;

        if (depth > 0 && depth < thickness1) {
            hit0 = 1;
            break;
        } else {
            search0 = search1;
        }
    }

    search1 = search0 + ((search1 - search0) / 2.0);

    steps *= hit0;

    for (i = 0; i < steps; ++i) {
        frag       = mix(startFrag.xy, endFrag.xy, search1);
        uv.xy      = frag / texSize;
        if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
            break;
        }
        positionTo = texture(u_TexPositionCamera, uv.xy);

        viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
        // My version of computing Z
        // q is search1
//        viewDistance = 1.f / mix(1.f / startView.z, 1.f / endView.z, search1);
        depth        = viewDistance - positionTo.z;
//        depth        = positionTo.z - viewDistance;

        if (depth > 0 && depth < thickness2) {
            hit1 = 1;
            search1 = search0 + ((search1 - search0) / 2);
        } else {
            float temp = search1;
            search1 = search1 + ((search1 - search0) / 2);
            search0 = temp;
        }
    }

    float visibility =
            hit1
            * positionTo.w
            * (1 - max( dot( -unitPositionFrom, pivot), 0))
            * (1 - clamp ( depth / thickness2, 0, 1))
            * (1 - clamp( length(positionTo - positionFrom) / maxDistance, 0, 1))
            * (1 - smoothstep(0.95f, 1.f, uv.y))
            * (smoothstep(0.f, 0.05f, uv.y))
            * (smoothstep(0.f, 0.05f, uv.x))
            * (1 - smoothstep(0.95f, 1.f, uv.x))
            ;

//    visibility = hit1 * positionTo.w;

    visibility = clamp(visibility, 0, 1);

    uv.ba = vec2(visibility);

    return visibility * texture(u_TexAlbedo, uv.xy).rgb;
//    return uv.rgb;
}
#endif
