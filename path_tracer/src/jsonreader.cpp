#include "jsonreader.h"
#include <iostream>
#include "scene/geometry/mesh.h"
#include "scene/geometry/shape.h"
#include "texture.h"

// Build scene as C++ constructs, then have each one write itself as ASCII
// to a QString that will be placed in the GLSL source

JSONReader::JSONReader(OpenGLContext *context)
    : m_shapeCounts(), mp_context(context),
      m_lowestUnusedArrayIdx(0), m_lowestUnusedTexSlot(4)
{}


bool JSONReader::LoadSceneFromFile(QFile &file, const QString &local_path, Scene &scene, Camera &gl_camera)
{
    if(file.open(QIODevice::ReadOnly))
    {
        scene.Clear();
        QByteArray rawData = file.readAll();
        // Parse document
        QJsonDocument doc(QJsonDocument::fromJson(rawData));

        // Get JSON object
        QJsonObject json = doc.object();
        QJsonObject sceneObj, camera;
        QJsonArray primitiveList, materialList, lightList;

        std::map<std::string, uPtr<Material>> mtl_name_to_material;
        QJsonArray frames = json["frames"].toArray();
        //check scene object for every frame
        for(const QJsonValue &frame : frames) {
            QJsonObject sceneObj = frame.toObject()["scene"].toObject();
            //load camera
            if(sceneObj.contains(QString("camera"))) {
                camera = sceneObj["camera"].toObject();
                gl_camera = LoadCamera(camera);
            }
            //load all materials in QMap with mtl name as key and Material itself as value
            if(sceneObj.contains(QString("materials"))){
                materialList = sceneObj["materials"].toArray();
                for(const QJsonValue &materialVal : materialList){
                    QJsonObject materialObj = materialVal.toObject();
                    LoadMaterial(materialObj, local_path, &mtl_name_to_material, &scene.textures);
                }
            }
            //load primitives and attach materials from QMap
            if(sceneObj.contains(QString("primitives"))) {
                primitiveList = sceneObj["primitives"].toArray();
                for(const QJsonValue &primitiveVal : primitiveList){
                    QJsonObject primitiveObj = primitiveVal.toObject();
                    LoadGeometry(primitiveObj, mtl_name_to_material, local_path, scene);
                }
            }
            //load lights and attach materials from QMap
            if(sceneObj.contains(QString("lights"))) {
                lightList = sceneObj["lights"].toArray();
                for(const QJsonValue &lightVal : lightList){
                    QJsonObject lightObj = lightVal.toObject();
                    LoadLights(lightObj, mtl_name_to_material, local_path, scene);
                }
            }
        }
        file.close();
        return true;
    }
    return false;
}

bool JSONReader::LoadGeometry(QJsonObject &geometry, const std::map<std::string, uPtr<Material>> &mtl_map, const QString &local_path, Scene &scene)
{
    //First check what type of geometry we're supposed to load
    QString type;
    if(geometry.contains(QString("shape"))){
        type = geometry["shape"].toString();
    }

    if(QString::compare(type, QString("Mesh")) == 0) {
        auto shape = mkU<Mesh>(this->mp_context, Mesh::nextLowestSamplerIndex++,
                               m_lowestUnusedTexSlot++);
        // Lets the new Mesh's triangles know its index
        // in the vector of meshes in Scene
        int currNumMeshes = scene.meshes.size();
        if(geometry.contains(QString("filename"))) {
            QString objFilePath = geometry["filename"].toString();
            shape->LoadOBJ(objFilePath, local_path, currNumMeshes);
        }
        if(geometry.contains(QString("transform"))) {
            QJsonObject transform = geometry["transform"].toObject();
            shape->transform = LoadTransform(transform);
        }

        if(geometry.contains(QString("material"))) {
            QString material_name = geometry["material"].toString();
            shape->material = mkU<Material>(*(mtl_map.at(material_name.toStdString())));
        }
        scene.meshes.push_back(std::move(shape));
    }
    else if(QString::compare(type, QString("Sphere")) == 0) {
        auto shape = mkU<Sphere>();
        if(geometry.contains(QString("transform"))) {
            QJsonObject transform = geometry["transform"].toObject();
            shape->transform = LoadTransform(transform);
        }

        if(geometry.contains(QString("material"))) {
            QString material_name = geometry["material"].toString();
            shape->material = mkU<Material>(*(mtl_map.at(material_name.toStdString())));
        }
        scene.spheres.push_back(std::move(shape));
    }
    else if(QString::compare(type, QString("SquarePlane")) == 0) {
        auto shape = mkU<RectangleShape>();
        if(geometry.contains(QString("transform"))) {
            QJsonObject transform = geometry["transform"].toObject();
            shape->transform = LoadTransform(transform);
        }

        if(geometry.contains(QString("material"))) {
            QString material_name = geometry["material"].toString();
            shape->material = mkU<Material>(*(mtl_map.at(material_name.toStdString())));
        }
        scene.rectangles.push_back(std::move(shape));
    }
    else if(QString::compare(type, QString("Cube")) == 0) {
        auto shape = mkU<Box>();
        if(geometry.contains(QString("transform"))) {
            QJsonObject transform = geometry["transform"].toObject();
            shape->transform = LoadTransform(transform);
        }

        if(geometry.contains(QString("material"))) {
            QString material_name = geometry["material"].toString();
            shape->material = mkU<Material>(*(mtl_map.at(material_name.toStdString())));
        }
        scene.boxes.push_back(std::move(shape));
    }
    else {
        std::cout << "Could not parse the geometry!" << std::endl;
        return NULL;
    }
    return true;
}

bool JSONReader::LoadLights(QJsonObject &geometry, const std::map<std::string, uPtr<Material>> &mtl_map, const QString &local_path, Scene &scene)
{
    // Determine if it's an area, point, or spot light
    if(geometry.contains(QString("type"))) {
        QString type = geometry["type"].toString();

        // If the light is a diffuse area light
        if(QString::compare(type, QString("DiffuseAreaLight")) == 0) {

            auto areaLight = mkU<AreaLight>();

            QString shapeType;
            if(geometry.contains(QString("shape"))) {
                shapeType = geometry["shape"].toString();

                if(QString::compare(shapeType, QString("Mesh")) == 0) {
                    // TODO handle mesh area lights
        #if 0
                    shape = std::make_shared<Mesh>();
                    if(geometry.contains(QString("filename"))) {
                        QString objFilePath = geometry["filename"].toString();
                        std::static_pointer_cast<Mesh>(shape)->LoadOBJ(objFilePath, local_path);
                    }
        #endif
                }
                else if(QString::compare(shapeType, QString("Sphere")) == 0) {
                    areaLight->shapeType = SPHERE_SHAPE;
                }
                else if(QString::compare(shapeType, QString("SquarePlane")) == 0) {
                    areaLight->shapeType = RECTANGLE_SHAPE;
                }
//                else if(QString::compare(shapeType, QString("Cube")) == 0) {
//                    areaLight->shapeType = BOX_SHAPE;
//                }
//                else if(QString::compare(shapeType, QString("Disc")) == 0) {
//                    areaLight->shapeType = DISC_SHAPE;
//                }
                else {
                    std::cout << "Could not parse the light!" << std::endl;
                    return NULL;
                }
            }
            // Load the light's transform
            if(geometry.contains(QString("transform"))) {
                QJsonObject transform = geometry["transform"].toObject();
                areaLight->transform = LoadTransform(transform);
            }
            // Load the light's color
            Color3f lightColor = ToVec3(geometry["lightColor"].toArray());
            Float intensity = static_cast< float >(geometry["intensity"].toDouble());
            areaLight->Le = lightColor * intensity;
            scene.areaLights.push_back(std::move(areaLight));
        }

        // If the light is a point light
        else if(QString::compare(type, QString("PointLight")) == 0) {
            auto pointLight = mkU<PointLight>();
            if(geometry.contains(QString("position"))) {
                pointLight->pos = ToVec3(geometry["position"].toArray());
            }

            // Load the light's transform
            if(geometry.contains(QString("transform"))) {
                QJsonObject transform = geometry["transform"].toObject();
                pointLight->transform = LoadTransform(transform);
            }
            // Load the light's color
            Color3f lightColor = ToVec3(geometry["lightColor"].toArray());
            Float intensity = static_cast< float >(geometry["intensity"].toDouble());
            pointLight->Le = lightColor * intensity;
            scene.pointLights.push_back(std::move(pointLight));
        }


        // If the light is a spot light
        else if(QString::compare(type, QString("SpotLight")) == 0) {
            auto spotLight = mkU<SpotLight>();
            if(geometry.contains(QString("position"))) {
                spotLight->pos = ToVec3(geometry["position"].toArray());
            }
            if(geometry.contains(QString("innerAngle"))) {
                spotLight->innerAngle = static_cast< float >(geometry["innerAngle"].toDouble());
            }
            if(geometry.contains(QString("outerAngle"))) {
                spotLight->outerAngle = static_cast< float >(geometry["outerAngle"].toDouble());
            }

            // Load the light's transform
            if(geometry.contains(QString("transform"))) {
                QJsonObject transform = geometry["transform"].toObject();
                spotLight->transform = LoadTransform(transform);
            }
            // Load the light's color
            Color3f lightColor = ToVec3(geometry["lightColor"].toArray());
            Float intensity = static_cast< float >(geometry["intensity"].toDouble());
            spotLight->Le = lightColor * intensity;
            scene.spotLights.push_back(std::move(spotLight));
        }
    }
    return true;
}

bool JSONReader::LoadMaterial(QJsonObject &material,
                              QString local_path,
                              std::map<std::string, uPtr<Material>> *mtl_map,
                              std::vector<uPtr<Texture>> *mtl_textures)
{
    QString type;

    //First check what type of material we're supposed to load
    if(material.contains(QString("type"))) type = material["type"].toString();

    if(QString::compare(type, QString("MatteMaterial")) == 0)
    {
        uPtr<Texture2D> textureMap;
        uPtr<Texture2D> normalMap;
        Color3f Kd = ToVec3(material["Kd"].toArray());
        Float sigma = static_cast< float >(material["sigma"].toDouble());
        if(material.contains(QString("textureMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["textureMap"].toString());
            textureMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            textureMap->create(img_filepath.toStdString().c_str(), false);
        }
        if(material.contains(QString("normalMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["normalMap"].toString());
            normalMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            normalMap->create(img_filepath.toStdString().c_str(), false);
        }
        auto result = mkU<Material>(Kd, sigma, -1., DIFFUSE_REFL,
                                    textureMap.get(), normalMap.get(), nullptr);
        if(textureMap) {
            mtl_textures->push_back(std::move(textureMap));
        }
        if(normalMap) {
            mtl_textures->push_back(std::move(normalMap));
        }
        mtl_map->insert({material["name"].toString().toStdString(), std::move(result)});
    }
    else if(QString::compare(type, QString("MirrorMaterial")) == 0)
    {
        uPtr<Texture2D> textureMap;
        uPtr<Texture2D> normalMap;
        uPtr<Texture2D> roughnessMap;

        Color3f Kr = ToVec3(material["Kr"].toArray());
        float roughness = 0.f;
        if(material.contains(QString("roughness"))) {
            roughness = material["roughness"].toDouble();
        }
        if(material.contains(QString("roughnessMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["roughnessMap"].toString());
            roughnessMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            roughnessMap->create(img_filepath.toStdString().c_str(), false);
        }
        if(material.contains(QString("textureMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["textureMap"].toString());
            textureMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            textureMap->create(img_filepath.toStdString().c_str(), false);
        }
        if(material.contains(QString("normalMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["normalMap"].toString());
            normalMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            normalMap->create(img_filepath.toStdString().c_str(), false);
        }

        uPtr<Material> result;
        if(roughness > 0) {
            result = mkU<Material>(Kr, roughness, -1., MICROFACET_REFL,
                                        textureMap.get(),
                                        normalMap.get(),
                                        roughnessMap.get());
        }
        else {
            result = mkU<Material>(Kr, roughness, -1., SPEC_REFL,
                                        textureMap.get(),
                                        normalMap.get(),
                                        roughnessMap.get());
        }
        if(textureMap) {
            mtl_textures->push_back(std::move(textureMap));
        }
        if(normalMap) {
            mtl_textures->push_back(std::move(normalMap));
        }
        if(roughnessMap) {
            mtl_textures->push_back(std::move(roughnessMap));
        }
        mtl_map->insert({material["name"].toString().toStdString(), std::move(result)});
    }
    else if(QString::compare(type, QString("TransmissiveMaterial")) == 0)
    {
        uPtr<Texture2D> textureMap;
        uPtr<Texture2D> normalMap;
        Color3f Kt = ToVec3(material["Kt"].toArray());
        float eta = material["eta"].toDouble();
        if(material.contains(QString("textureMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["textureMap"].toString());
            textureMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            textureMap->create(img_filepath.toStdString().c_str(), false);
        }
        if(material.contains(QString("normalMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["normalMap"].toString());
            normalMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            normalMap->create(img_filepath.toStdString().c_str(), false);
        }
        auto result = mkU<Material>(Kt, 0, eta, SPEC_TRANS,
                                    textureMap.get(), normalMap.get(), nullptr);

        if(textureMap) {
            mtl_textures->push_back(std::move(textureMap));
        }
        if(normalMap) {
            mtl_textures->push_back(std::move(normalMap));
        }
        mtl_map->insert({material["name"].toString().toStdString(), std::move(result)});
    }
    else if(QString::compare(type, QString("GlassMaterial")) == 0)
    {
        uPtr<Texture2D> textureMap;
        uPtr<Texture2D> normalMap;
        Color3f Kr = ToVec3(material["Kr"].toArray());
        Color3f Kt = ToVec3(material["Kt"].toArray());
        float eta = material["eta"].toDouble();
//        if(material.contains(QString("textureMapRefl"))) {
//            QString img_filepath = local_path.append(material["textureMapRefl"].toString());
//            textureMapRefl = std::make_shared<QImage>(img_filepath);
//        }
        if(material.contains(QString("textureMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["textureMap"].toString());
            textureMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            textureMap->create(img_filepath.toStdString().c_str(), false);
        }
        if(material.contains(QString("normalMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["normalMap"].toString());
            normalMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            normalMap->create(img_filepath.toStdString().c_str(), false);
        }
        auto result = mkU<Material>(Kt, 0, eta, SPEC_GLASS,
                                    textureMap.get(), normalMap.get(), nullptr);

        if(textureMap) {
            mtl_textures->push_back(std::move(textureMap));
        }
        if(normalMap) {
            mtl_textures->push_back(std::move(normalMap));
        }
        mtl_map->insert({material["name"].toString().toStdString(), std::move(result)});
    }
    else if(QString::compare(type, QString("PlasticMaterial")) == 0)
    {
        uPtr<Texture2D> textureMap;
        uPtr<Texture2D> normalMap;
        uPtr<Texture2D> roughnessMap;

        Color3f Kd = ToVec3(material["Kd"].toArray());
        Color3f Ks = ToVec3(material["Ks"].toArray());
        float roughness = material["roughness"].toDouble();
        if(material.contains(QString("roughnessMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["roughnessMap"].toString());
            roughnessMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            roughnessMap->create(img_filepath.toStdString().c_str(), false);
        }
        if(material.contains(QString("textureMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["textureMap"].toString());
            textureMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            textureMap->create(img_filepath.toStdString().c_str(), false);
        }
//        if(material.contains(QString("textureMapSpecular"))) {
//            QString img_filepath = local_path.append(material["textureMapSpecular"].toString());
//            textureMapSpecular = std::make_shared<QImage>(img_filepath);
//        }
        if(material.contains(QString("normalMap"))) {
            QString img_filepath = local_path;
            img_filepath.append(material["normalMap"].toString());
            normalMap = mkU<Texture2D>(mp_context, m_lowestUnusedArrayIdx++, m_lowestUnusedTexSlot++);
            normalMap->create(img_filepath.toStdString().c_str(), false);
        }
        auto result = mkU<Material>(Kd, roughness, -1, PLASTIC,
                                    textureMap.get(),
                                    normalMap.get(),
                                    roughnessMap.get());
        if(textureMap) {
            mtl_textures->push_back(std::move(textureMap));
        }
        if(normalMap) {
            mtl_textures->push_back(std::move(normalMap));
        }
        if(roughnessMap) {
            mtl_textures->push_back(std::move(roughnessMap));
        }
        mtl_map->insert({material["name"].toString().toStdString(), std::move(result)});
    }
    else
    {
        std::cout << "Could not parse the material!" << std::endl;
        return false;
    }

    return true;
}

Camera JSONReader::LoadCamera(QJsonObject& camera)
{
    Camera result;
    if(camera.contains(QString("target"))) result.ref = ToVec3(camera["target"].toArray());
    if(camera.contains(QString("eye"))) result.eye = ToVec3(camera["eye"].toArray());
    if(camera.contains(QString("worldUp"))) result.world_up = ToVec3(camera["worldUp"].toArray());
    if(camera.contains(QString("width"))) result.width = camera["width"].toDouble();
    if(camera.contains(QString("height"))) result.height = camera["height"].toDouble();
    if(camera.contains(QString("fov"))) result.fovy = camera["fov"].toDouble();
    if(camera.contains(QString("nearClip"))) result.near_clip = camera["nearClip"].toDouble();
    if(camera.contains(QString("farClip"))) result.far_clip = camera["farClip"].toDouble();

    result.RecomputeAttributes();
    return result;
}

Transform JSONReader::LoadTransform(QJsonObject &transform)
{
    Vector3f t, r, s;
    s = Vector3f(1,1,1);
    if(transform.contains(QString("translate"))) t = ToVec3(transform["translate"].toArray());
    if(transform.contains(QString("rotate"))) r = ToVec3(transform["rotate"].toArray());
    if(transform.contains(QString("scale"))) s = ToVec3(transform["scale"].toArray());
    return Transform(t, r, s);
}

glm::vec3 JSONReader::ToVec3(const QJsonArray &s)
{
    glm::vec3 result(s.at(0).toDouble(), s.at(1).toDouble(), s.at(2).toDouble());
    return result;
}

glm::vec3 JSONReader::ToVec3(const QString &s)
{
    glm::vec3 result;
    int start_idx;
    int end_idx = -1;
    for(int i = 0; i < 3; i++){
        start_idx = ++end_idx;
        while(end_idx < s.length() && s.at(end_idx) != QChar(' '))
        {
            end_idx++;
        }
        result[i] = s.mid(start_idx, end_idx - start_idx).toFloat();
    }
    return result;
}

