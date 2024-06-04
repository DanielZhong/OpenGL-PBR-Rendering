// optimized algorithm for solving quadratic equations developed by Dr. Po-Shen Loh -> https://youtu.be/XKBX0r3J-9Y
// Adapted to root finding (ray t0/t1) for all quadric shapes (sphere, ellipsoid, cylinder, cone, etc.) by Erich Loftis
void solveQuadratic(float A, float B, float C, out float t0, out float t1) {
    float invA = 1.0 / A;
    B *= invA;
    C *= invA;
    float neg_halfB = -B * 0.5;
    float u2 = neg_halfB * neg_halfB - C;
    float u = u2 < 0.0 ? neg_halfB = 0.0 : sqrt(u2);
    t0 = neg_halfB - u;
    t1 = neg_halfB + u;
}

vec2 sphereUVMap(vec3 p) {
    float phi = atan(p.z, p.x);
    if(phi < 0) {
        phi += TWO_PI;
    }
    float theta = acos(p.y);
    return vec2(1 - phi/TWO_PI, 1 - theta / PI);
}

float sphereIntersect(Ray ray, float radius, vec3 pos, out vec3 localNor, out vec2 out_uv, mat4 invT) {
    ray.origin = vec3(invT * vec4(ray.origin, 1.));
    ray.direction = vec3(invT * vec4(ray.direction, 0.));
    float t0, t1;
    vec3 diff = ray.origin - pos;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(ray.direction, diff);
    float c = dot(diff, diff) - (radius * radius);
    solveQuadratic(a, b, c, t0, t1);
    localNor = t0 > 0.0 ? ray.origin + t0 * ray.direction : ray.origin + t1 * ray.direction;
    localNor = normalize(localNor);
    out_uv = sphereUVMap(localNor);
    return t0 > 0.0 ? t0 : t1 > 0.0 ? t1 : INFINITY;
}

float planeIntersect( vec4 pla, vec3 rayOrigin, vec3 rayDirection, mat4 invT) {
    rayOrigin = vec3(invT * vec4(rayOrigin, 1.));
    rayDirection = vec3(invT * vec4(rayDirection, 0.));
    vec3 n = pla.xyz;
    float denom = dot(n, rayDirection);

    vec3 pOrO = (pla.w * n) - rayOrigin;
    float result = dot(pOrO, n) / denom;
    return (result > 0.0) ? result : INFINITY;
}

float rectangleIntersect(vec3 pos, vec3 normal,
                         float radiusU, float radiusV,
                         vec3 rayOrigin, vec3 rayDirection,
                         out vec2 out_uv, mat4 invT) {
    rayOrigin = vec3(invT * vec4(rayOrigin, 1.));
    rayDirection = vec3(invT * vec4(rayDirection, 0.));
    float dt = dot(-normal, rayDirection);
    // use the following for one-sided rectangle
    if (dt < 0.0) return INFINITY;
    float t = dot(-normal, pos - rayOrigin) / dt;
    if (t < 0.0) return INFINITY;

    vec3 hit = rayOrigin + rayDirection * t;
    vec3 vi = hit - pos;
    vec3 U = normalize( cross( abs(normal.y) < 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0), normal ) );
    vec3 V = cross(normal, U);

    out_uv = vec2(dot(U, vi) / length(U), dot(V, vi) / length(V));
    out_uv = out_uv + vec2(0.5, 0.5);

    return (abs(dot(U, vi)) > radiusU || abs(dot(V, vi)) > radiusV) ? INFINITY : t;
}

float boxIntersect(vec3 minCorner, vec3 maxCorner,
                   mat4 invT, mat3 invTransT,
                   vec3 rayOrigin, vec3 rayDirection,
                   out vec3 normal, out bool isRayExiting,
                   out vec2 out_uv) {
        rayOrigin = vec3(invT * vec4(rayOrigin, 1.));
        rayDirection = vec3(invT * vec4(rayDirection, 0.));
        vec3 invDir = 1.0 / rayDirection;
        vec3 near = (minCorner - rayOrigin) * invDir;
        vec3 far  = (maxCorner - rayOrigin) * invDir;
        vec3 tmin = min(near, far);
        vec3 tmax = max(near, far);
        float t0 = max( max(tmin.x, tmin.y), tmin.z);
        float t1 = min( min(tmax.x, tmax.y), tmax.z);
        if (t0 > t1) return INFINITY;
        if (t0 > 0.0) // if we are outside the box
        {
                normal = -sign(rayDirection) * step(tmin.yzx, tmin) * step(tmin.zxy, tmin);
                normal = normalize(invTransT * normal);
                isRayExiting = false;
                vec3 p = t0 * rayDirection + rayOrigin;
                p = (p - minCorner) / (maxCorner - minCorner);
                out_uv = p.xy;
                return t0;
        }
        if (t1 > 0.0) // if we are inside the box
        {
                normal = -sign(rayDirection) * step(tmax, tmax.yzx) * step(tmax, tmax.zxy);
                normal = normalize(invTransT * normal);
                isRayExiting = true;
                vec3 p = t1 * rayDirection + rayOrigin;
                p = (p - minCorner) / (maxCorner - minCorner);
                out_uv = p.xy;
                return t1;
        }
        return INFINITY;
}

// Möller–Trumbore intersection
float triangleIntersect(vec3 p0, vec3 p1, vec3 p2,
                        vec3 rayOrigin, vec3 rayDirection) {
    const float EPSILON = 0.0000001;
    vec3 edge1, edge2, h, s, q;
    float a,f,u,v;
    edge1 = p1 - p0;
    edge2 = p2 - p0;
    h = cross(rayDirection, edge2);
    a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON) {
        return INFINITY;    // This ray is parallel to this triangle.
    }
    f = 1.0/a;
    s = rayOrigin - p0;
    u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return INFINITY;
    q = cross(s, edge1);
    v = f * dot(rayDirection, q);
    if (v < 0.0 || u + v > 1.0) {
        return INFINITY;
    }
    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * dot(edge2, q);
    if (t > EPSILON) {
        return t;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return INFINITY;
}

vec3 barycentric(vec3 p, vec3 t1, vec3 t2, vec3 t3) {
    vec3 edge1 = t2 - t1;
    vec3 edge2 = t3 - t2;
    float S = length(cross(edge1, edge2));

    edge1 = p - t2;
    edge2 = p - t3;
    float S1 = length(cross(edge1, edge2));

    edge1 = p - t1;
    edge2 = p - t3;
    float S2 = length(cross(edge1, edge2));

    edge1 = p - t1;
    edge2 = p - t2;
    float S3 = length(cross(edge1, edge2));

    return vec3(S1 / S, S2 / S, S3 / S);
}

#if N_MESHES
float meshIntersect(int mesh_id,
                    vec3 rayOrigin, vec3 rayDirection,
                    out vec3 out_nor, out vec2 out_uv,
                    mat4 invT) {

    rayOrigin = vec3(invT * vec4(rayOrigin, 1.));
    rayDirection = vec3(invT * vec4(rayDirection, 0.));

    int sampIdx = 0;// meshes[mesh_id].triangle_sampler_index;

    float t = INFINITY;

    // Iterate over each triangle, and
    // convert it to a pixel coordinate
    for(int i = 0; i < meshes[mesh_id].num_tris; ++i) {
        // pos0, pos1, pos2, nor0, nor1, nor2, uv0, uv1, uv2
        // Each triangle takes up 9 pixels
        Triangle tri;
        int first_pixel = i * 9;
        // Positions
        for(int p = first_pixel; p < first_pixel + 3; ++p) {
            int row = int(floor(float(p) / meshes[mesh_id].triangle_storage_side_len));
            int col = p - row * meshes[mesh_id].triangle_storage_side_len;

            tri.pos[p - first_pixel] = texelFetch(u_TriangleStorageSamplers[sampIdx],
                                                ivec2(col, row), 0).rgb;
        }
        first_pixel += 3;
        // Normals
        for(int n = first_pixel; n < first_pixel + 3; ++n) {
            int row = int(floor(float(n) / meshes[mesh_id].triangle_storage_side_len));
            int col = n - row * meshes[mesh_id].triangle_storage_side_len;

            tri.nor[n - first_pixel] = texelFetch(u_TriangleStorageSamplers[sampIdx],
                                                ivec2(col, row), 0).rgb;
        }
        first_pixel += 3;
        // UVs
        for(int v = first_pixel; v < first_pixel + 3; ++v) {
            int row = int(floor(float(v) / meshes[mesh_id].triangle_storage_side_len));
            int col = v - row * meshes[mesh_id].triangle_storage_side_len;

            tri.uv[v - first_pixel] = texelFetch(u_TriangleStorageSamplers[sampIdx],
                                               ivec2(col, row), 0).rg;
        }

        float d = triangleIntersect(tri.pos[0], tri.pos[1], tri.pos[2],
                                    rayOrigin, rayDirection);
        if(d < t) {
            t = d;
            vec3 p = rayOrigin + t * rayDirection;
            vec3 baryWeights = barycentric(p, tri.pos[0], tri.pos[1], tri.pos[2]);
            out_nor = baryWeights[0] * tri.nor[0] +
                      baryWeights[1] * tri.nor[1] +
                      baryWeights[2] * tri.nor[2];
            out_uv =  baryWeights[0] * tri.uv[0] +
                      baryWeights[1] * tri.uv[1] +
                      baryWeights[2] * tri.uv[2];
        }
    }

    return t;
}
#endif

Intersection sceneIntersect(Ray ray) {
    float t = INFINITY;
    Intersection result;
    result.t = INFINITY;

#if N_RECTANGLES
    for(int i = 0; i < N_RECTANGLES; ++i) {
        vec2 uv;
        float d = rectangleIntersect(rectangles[i].pos, rectangles[i].nor,
                                     rectangles[i].halfSideLengths.x,
                                     rectangles[i].halfSideLengths.y,
                                     ray.origin, ray.direction,
                                     uv,
                                     rectangles[i].transform.invT);
        if(d < t) {
            t = d;
            result.t = t;
            result.nor = normalize(rectangles[i].transform.invTransT * rectangles[i].nor);
            result.uv = uv;
            result.Le = vec3(0,0,0);
            result.obj_ID = rectangles[i].ID;
            result.material = rectangles[i].material;
        }
    }
#endif
#if N_BOXES
    for(int i = 0; i < N_BOXES; ++i) {
        vec3 nor;
        bool isExiting;
        vec2 uv;
        float d = boxIntersect(boxes[i].minCorner, boxes[i].maxCorner,
                               boxes[i].transform.invT, boxes[i].transform.invTransT,
                               ray.origin, ray.direction,
                               nor, isExiting, uv);
        if(d < t) {
            t = d;
            result.t = t;
            result.nor = nor;
            result.Le = vec3(0,0,0);
            result.obj_ID = boxes[i].ID;
            result.material = boxes[i].material;
            result.uv = uv;
        }
    }
#endif
#if N_SPHERES
    for(int i = 0; i < N_SPHERES; ++i) {
        vec3 nor;
        bool isExiting;
        vec3 localNor;
        vec2 uv;
        float d = sphereIntersect(ray, spheres[i].radius, spheres[i].pos, localNor, uv,
                                  spheres[i].transform.invT);
        if(d < t) {
            t = d;
            vec3 p = ray.origin + t * ray.direction;
            result.t = t;
            result.nor = normalize(spheres[i].transform.invTransT * localNor);
            result.Le = vec3(0,0,0);
            result.uv = uv;
            result.obj_ID = spheres[i].ID;
            result.material = spheres[i].material;
        }
    }
#endif
#if N_MESHES
    for(int i = 0; i < N_MESHES; ++i) {
        vec3 nor;
        vec2 uv;
        float d = meshIntersect(i, ray.origin, ray.direction,
                                nor, uv, meshes[i].transform.invT);

        if(d < t) {
            t = d;
            result.t = t;
            result.nor = nor;
            result.uv =  uv;
            result.Le = vec3(0,0,0);
            result.obj_ID = meshes[i].ID;
            result.material = meshes[i].material;
        }
    }
#endif
#if N_AREA_LIGHTS
    for(int i = 0; i < N_AREA_LIGHTS; ++i) {
        int shapeType = areaLights[i].shapeType;
        if(shapeType == RECTANGLE) {
            vec3 pos = vec3(0,0,0);
            vec3 nor = vec3(0,0,1);
            vec2 halfSideLengths = vec2(0.5, 0.5);
            vec2 uv;
            float d = rectangleIntersect(pos, nor,
                                   halfSideLengths.x,
                                   halfSideLengths.y,
                                   ray.origin, ray.direction,
                                   uv,
                                   areaLights[i].transform.invT);
            if(d < t) {
                t = d;
                result.t = t;
                result.nor = normalize(areaLights[i].transform.invTransT * vec3(0,0,1));
                result.Le = areaLights[i].Le;
                result.obj_ID = areaLights[i].ID;
            }
        }
        else if(shapeType == SPHERE) {
            vec3 pos = vec3(0,0,0);
            float radius = 1.;
            mat4 invT = areaLights[i].transform.invT;
            vec3 localNor;
            vec2 uv;
            float d = sphereIntersect(ray, radius, pos, localNor, uv, invT);
            if(d < t) {
                t = d;
                result.t = t;
                result.nor = normalize(areaLights[i].transform.invTransT * localNor);
                result.Le = areaLights[i].Le;
                result.obj_ID = areaLights[i].ID;
            }
        }
    }
#endif
#if N_TEXTURES
    if(result.material.normalTex != -1) {
        vec3 localNor = texture(u_TexSamplers[result.material.normalTex], result.uv).rgb;
        localNor = localNor * 2. - vec3(1.);
        vec3 tan, bit;
        coordinateSystem(result.nor, tan, bit);
        result.nor = mat3(tan, bit, result.nor) * localNor;
    }
#endif
    return result;
}

Intersection areaLightIntersect(AreaLight light, Ray ray) {
    Intersection result;
    result.t = INFINITY;
#if N_AREA_LIGHTS
    int shapeType = light.shapeType;
    if(shapeType == RECTANGLE) {
        vec3 pos = vec3(0,0,0);
        vec3 nor = vec3(0,0,1);
        vec2 halfSideLengths = vec2(0.5, 0.5);
        vec2 uv;
        float d = rectangleIntersect(pos, nor,
                               halfSideLengths.x,
                               halfSideLengths.y,
                               ray.origin, ray.direction,
                               uv,
                               light.transform.invT);
        result.t = d;
        result.nor = normalize(light.transform.invTransT * vec3(0,0,1));
        result.Le = light.Le;
        result.obj_ID = light.ID;
    }
    else if(shapeType == SPHERE) {
        vec3 pos = vec3(0,0,0);
        float radius = 1.;
        mat4 invT = light.transform.invT;
        vec3 localNor;
        vec2 uv;
        float d = sphereIntersect(ray, radius, pos, localNor, uv, invT);
        result.t = d;
        result.nor = normalize(light.transform.invTransT * localNor);
        result.Le = light.Le;
        result.obj_ID = light.ID;
    }
#endif
    return result;
}
