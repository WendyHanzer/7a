#version 330

in vec2 texcoord;
uniform sampler2D tex;
uniform vec4 color;
out vec4 glColor;

void main(void) {
    glColor = vec4(1,1,1, texture2D(tex, texcoord).a) * color;
}
