#version 430
layout(location = 0) out vec4 fragColor;
in vec3 tePosition;

void main() {

//fragColor = vec4(vec3(1.0,0.0,0.0), 1.0);
fragColor = vec4(tePosition.xyz, 1.0);
}