#version 410
//uniform bool hasTexture;
//varying vec2 tex_coords;
uniform sampler1D tex;
in float colorPos;
out vec4 glColor;

void main(void) {
    vec4 color = texture(tex, colorPos);

    glColor = color;
}
