#version 330

layout(location = 0) out float fragmentdepth;

void main() {
    // just take the depth of the fragment
	fragmentdepth = gl_FragCoord.z;
}
