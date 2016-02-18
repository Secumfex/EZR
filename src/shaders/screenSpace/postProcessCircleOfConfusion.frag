#version 430

in vec2 passUV;

uniform sampler2D colorMap;
// uniform sampler2D depthMap;
uniform sampler2D positionMap;

// near-blurry, near-sharp, far-sharp, far-blurry
uniform vec4 focusPlaneDepths;
uniform vec2 focusPlaneRadi; // radius of near-blurry and radius of far-blurry

out vec4 fragmentColor;

void main() {
    // float depth = texture(depthMap, passUV).r;
    vec3 color = texture(colorMap,passUV).rgb;
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
		float t = clamp( (depth - nearBlurryDepth) / (nearSharpDepth - nearBlurryDepth), 0.0, 1.0);
		cocRadius = mix(nearBlurryRadius, nearSharpRadius, t);
	}
	if (depth > nearSharpDepth && depth < farSharpDepth) // in sharp area
	{ 

		cocRadius = max(min(mix(nearSharpRadius, farSharpRadius, (depth-nearSharpDepth)/(farSharpDepth-nearSharpDepth)),nearSharpRadius),farSharpRadius);
	}
	if (depth > farSharpDepth)
	{ 
		cocRadius = max(mix(farSharpRadius, farBlurryRadius, (depth-farSharpDepth)/(farBlurryDepth-farSharpDepth)), farBlurryRadius);
	}

	fragmentColor = vec4(color,cocRadius);
}    