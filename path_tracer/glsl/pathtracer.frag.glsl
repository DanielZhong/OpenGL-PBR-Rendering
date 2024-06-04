
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
