#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

uniform mat4 model;

uniform vec3 objectScale;
uniform bool useTextureScaling;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;
    if(useTextureScaling){
        vec3 absScale = abs(objectScale);
        vec3 absNormal = abs(aNormal);// getting dominat axis
        if (absNormal.y > absNormal.x && absNormal.y > absNormal.z) {
            // fro y -> xz
            vs_out.TexCoords = (vec2(aPos.x, aPos.z) + 1.0) * (absScale.xz * 0.5);
        } 
        else if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
            // for x -> yz
            vs_out.TexCoords = (vec2(aPos.y, aPos.z) + 1.0) * (absScale.yz * 0.5);
        } 
        else {
            // for z -> xy
            vs_out.TexCoords = (vec2(aPos.x, aPos.y) + 1.0) * (absScale.xy * 0.5);
        }
    }else{
        vs_out.TexCoords = aTexCoords;
    }
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
