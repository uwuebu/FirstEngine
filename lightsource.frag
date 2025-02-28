#version 420 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D lightTexture;

void main() {
    FragColor = texture(lightTexture, TexCoord);
}
