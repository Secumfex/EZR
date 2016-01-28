#version 430

in vec2 passUV;

// uniform sampler2D colorMap;
// uniform sampler2D depthMap;
uniform sampler2D positionMap;

// near-blurry, near-sharp, far-sharp, far-blurry
uniform vec4 focusPlaneDepths;
uniform vec2 focusPlaneRadi; // radius of near-blurry and radius of far-blurry
const vec2 sharpPlaneRadi = vec2(-0.5,0.5);

out vec4 fragmentColor;

void main() {
    // float depth = texture(depthMap, passUV).r;
    float depth = length(texture(positionMap, passUV).xyz);

    float nearBlurryDepth = focusPlaneDepths.x;
    float nearSharpDepth  = focusPlaneDepths.y;
    float farSharpDepth   = focusPlaneDepths.z;
    float farBlurryDepth  = focusPlaneDepths.w;

    float nearBlurryRadius = focusPlaneRadi.x;
    float nearSharpRadius  = 0.5;
    float farSharpRadius   = -0.5;
    float farBlurryRadius  = focusPlaneRadi.y;

	float cocRadius = 0.0;
	if (depth < nearSharpDepth)
	{ 								// 10.0				0.5				d - 0.0 / nearSharp
		cocRadius = min(mix(nearBlurryRadius, nearSharpRadius, (depth-nearBlurryDepth)/(nearSharpDepth-nearBlurryDepth)),nearBlurryRadius);
	}
	if (depth > nearSharpDepth && depth < farSharpDepth)
	{ 
		cocRadius = max(min(mix(nearSharpRadius, farSharpRadius, (depth-nearSharpDepth)/(farSharpDepth-nearSharpDepth)),nearSharpRadius),farSharpRadius);
	}
	if (depth > farSharpDepth)
	{ 
		cocRadius = max(mix(farSharpRadius, farBlurryRadius, (depth-farSharpDepth)/(farBlurryDepth-farSharpDepth)), farBlurryRadius);
	}

	fragmentColor = vec4(cocRadius);
}    