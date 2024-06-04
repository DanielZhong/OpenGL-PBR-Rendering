
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
