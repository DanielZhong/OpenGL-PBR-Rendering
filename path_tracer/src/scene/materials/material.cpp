#include "material.h"

Material::Material()
    : albedo(), roughness(), eta(), type(),
      albedoTex(nullptr), normalTex(nullptr), roughnessTex(nullptr)
{}

Material::Material(glm::vec3 albedo, float roughness, float eta, int type,
         Texture2D *albedoTex, Texture2D *normalTex,
         Texture2D *roughnessTex)
    : albedo(albedo), roughness(roughness), eta(eta), type(type),
      albedoTex(albedoTex),
      normalTex(normalTex),
      roughnessTex(roughnessTex)
{}

Material::Material(const Material &m)
    : albedo(m.albedo), roughness(m.roughness), eta(m.eta), type(m.type),
      albedoTex(m.albedoTex),
      normalTex(m.normalTex),
      roughnessTex(m.roughnessTex)
{}

Material& Material::operator=(const Material &m2) {
    this->albedo = m2.albedo;
    this->roughness = m2.roughness;
    this->eta = m2.eta;
    this->type = m2.type;
    this->albedoTex = m2.albedoTex;
    this->normalTex = m2.normalTex;
    this->roughnessTex = m2.roughnessTex;
    return *this;
}

QString Material::toGLSL() const {
    return "Material(vec3(" +
            QString::number(albedo.r) + ", " +
            QString::number(albedo.g) + ", " +
            QString::number(albedo.b) + "), " +
            QString::number(roughness) + ", " +
            QString::number(eta) + ", " +
            QString::number(type) + ", " +
            QString::number(albedoTex ? albedoTex->m_uniformArrayIndex : -1) + ", " +
            QString::number(normalTex ? normalTex->m_uniformArrayIndex : -1) + ", " +
            QString::number(roughnessTex ? roughnessTex->m_uniformArrayIndex : -1) + ")";
}
