#version 330

layout(location = 1) out float fragmentdepth;

void main() {
    // just take the depth of the fragment
	fragmentdepth = gl_FragCoord.z;
}
