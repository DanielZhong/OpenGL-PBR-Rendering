
vec3 squareToDiskConcentric(vec2 sample) {
    vec2 uOffset = 2.0 * sample - vec2(1.0, 1.0);

    if (uOffset.x == 0.0 && uOffset.y == 0.0) {
        return vec3(0.0, 0.0, 0.0);
    }

    float theta, r;
    if (abs(uOffset.x) > abs(uOffset.y)) {
        r = uOffset.x;
        theta = PI / 4.0 * (uOffset.y / uOffset.x);
    } else {
        r = uOffset.y;
        theta = PI / 2.0 - PI / 4.0 * (uOffset.x / uOffset.y);
    }

    return r * vec3(cos(theta), sin(theta), 0.0);
}

vec3 squareToHemisphereCosine(vec2 sample) {
    vec2 disk = vec2(squareToDiskConcentric(sample));
    float x = disk.x;
    float y = disk.y;
    float zSquared = max(0.0, 1.0 - x * x - y * y);
    float z = sqrt(zSquared);
    return vec3(x, y, z);
}

float squareToHemisphereCosinePDF(vec3 sample) {
    return CosTheta(sample) * INV_PI;
}

vec3 squareToSphereUniform(vec2 sample) {
    float z = 1.0 - 2.0 * sample.x;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = TWO_PI * sample.y;
    float x = r * cos(phi);
    float y = r * sin(phi);
    return vec3(x, y, z);
}

float squareToSphereUniformPDF(vec3 sample) {
    return INV_FOUR_PI;
}
