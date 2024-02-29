#version 330 core

uniform sampler2D u_Texture; // An object that lets us access
                             // the texture file we project onto
                             // our model's surface
uniform vec3 u_CamLook; // The camera's forward vector
uniform vec3 u_CamPos; // The camera's position

in vec3 fs_Pos;
in vec3 fs_Nor;
in vec2 fs_UV;

out vec3 out_Color;

void main() {
    // Adjustable parameters
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 ambientColor = vec3(0.1, 0.1, 0.1);
    vec3 specularColor = vec3(1.0, 1.0, 1.0);
    float shininess = 32.0;

    // variable
    vec3 norm = normalize(fs_Nor);
    vec3 lightDir = normalize(-u_CamLook);
    vec3 albedo = texture(u_Texture, fs_UV).rgb;

    // Ambient component
    vec3 ambient = ambientColor * albedo;

    // Diffuse component using the camera's forward vector as light direction
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * albedo;

    // Specular component
    vec3 viewDir = normalize(u_CamPos - fs_Pos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularColor * spec * lightColor;

    // Combine all components
    out_Color = ambient + diffuse + specular;
}
