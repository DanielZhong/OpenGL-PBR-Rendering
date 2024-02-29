#pragma once
#include <polygon.h>
#include <QImage>
#include "camera.h"

enum class ShadingModel : uint8_t
{
    Phong,
    BlinnPhong
};

class Rasterizer
{
private:
    //This is the set of Polygons loaded from a JSON scene file
    std::vector<Polygon> m_polygons;
    Camera m_camera;
    ShadingModel shadingModel = ShadingModel::BlinnPhong;
public:
    Rasterizer(const std::vector<Polygon>& polygons);
    QImage RenderScene();
    void ClearScene();
    Camera& GetCamera();

    void setShadingModel(ShadingModel inShadingModel);
    int scalingFactor = 1;
};
