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
