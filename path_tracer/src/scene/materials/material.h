#pragma once
#include <globals.h>
#include <QString>
#include "texture.h"


// Material types
#define DIFFUSE_REFL    1
#define SPEC_REFL       2
#define SPEC_TRANS      3
#define SPEC_GLASS      4
#define MICROFACET_REFL 5
#define PLASTIC         6
#define DIFFUSE_TRANS   7

class Texture2D;

// A Material is an interface class designed to produce a
// temporary BSDF for a given Intersection with the Primitive
// to which it is attached.
// Depending on the type of Material producing the BSDF, it
// may produce several BxDFs attached to the BSDF. For example,
// a GlassMaterial would produce a BSDF containing a specular
// reflection BRDF and a specular transmission BTDF.
struct Material {
    glm::vec3 albedo;
    float roughness;
    float eta; // Only for transmissive materials
    int type;

    // There will be an array of
    // sampler2Ds in the shader,
    // and this is the index of
    // the sampler to read from
    // if this material has a texture.
    // Will be -1 if there is no texture.
    //int albedoTex, normalTex, roughnessTex;

    Texture2D *albedoTex, *normalTex, *roughnessTex;

    Material();

    Material(glm::vec3 albedo, float roughness, float eta, int type,
             Texture2D *albedoTex, Texture2D *normalTex,
             Texture2D *roughnessTex);

    Material(const Material &m);
    Material& operator=(const Material &m2);

    QString toGLSL() const;
};
