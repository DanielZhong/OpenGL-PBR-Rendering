#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QList>
#include <unordered_map>

#include "scene/scene.h"
#include "openglcontext.h"

enum ShapeType {
    SQUARE, SPHERE, BOX, DISC, TRIANGLE
};

struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

class JSONReader {
private:
    // Each Shape that appears in a loaded scene will have a KVP in this map
    // The value stores how many instances of that shape type are in the scene
    std::unordered_map<ShapeType, unsigned int, EnumHash> m_shapeCounts;
    // For instantiating Textures, needed by Materials
    OpenGLContext *mp_context;
    // For tracking how many textures we've made so far
    unsigned int m_lowestUnusedArrayIdx;
    unsigned int m_lowestUnusedTexSlot;

public:
    JSONReader(OpenGLContext*);

    bool LoadSceneFromFile(QFile &file, const QString &local_path, Scene &scene, Camera &gl_camera);
    bool LoadGeometry(QJsonObject &geometry, const std::map<std::string, uPtr<Material> > &mtl_map, const QString &local_path, Scene &scene);
    bool LoadLights(QJsonObject &geometry, const std::map<std::string, uPtr<Material>> &mtl_map, const QString &local_path, Scene &scene);
    bool LoadMaterial(QJsonObject &material, QString local_path, std::map<std::string, uPtr<Material> > *mtl_map, std::vector<std::unique_ptr<Texture> > *mtl_textures);
    Camera LoadCamera(QJsonObject &camera);
    Transform LoadTransform(QJsonObject &transform);
    glm::vec3 ToVec3(const QJsonArray &s);
    glm::vec3 ToVec3(const QString &s);
};
