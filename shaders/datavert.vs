#version 410
in vec3 v_position;
//in vec2 v_texCoord;
//in vec3 v_color;
//in vec3 v_normal;

uniform mat4 mvpMatrix;

//uniform sampler2D tex;
uniform float heightScalar;
out float colorPos;

in float dataPoint;

void main(void) {
    // get vertex position
    vec3 newPos = v_position;
    newPos.y = newPos.y * heightScalar;
    vec4 pos = (mvpMatrix * vec4(newPos,1.0));

    colorPos = dataPoint - 0.35;//v_position.y - 0.35;
    // set vertex position
    gl_Position = pos;//vec4(pos, 1.0f);//mvpMatrix * vec4(v_position, 1.0);
}
