#version 410
in vec3 v_position;

uniform mat4 mvpMatrix;
uniform float heightScalar;

void main(void) {
    vec3 newPos = v_position;
    newPos.y = v_position.y * heightScalar;
    gl_Position = (mvpMatrix * vec4(newPos,1.0));
}
