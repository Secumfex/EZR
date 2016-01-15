#version 430

//incoming data for the single textures
in vec3 passPosition;
in vec2 passUVCoord;
in vec3 passNormal;

in VertexData {
	vec2 texCoord;
	vec3 position;
	vec3 normal;
} VertexOut;

uniform vec4  color;
uniform float mixTexture;
uniform sampler2D tex;

unfiorm bool hasNormalTex;
in vec3 passNormalWorld;
in vec3 passTangentWorld;
uniform sampler2D normalTex;
uniform mat4 view;

//writable textures for deferred screen space calculations
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out vec4 fragPosition;
layout(location = 3) out vec4 fragUVCoord;
 
void main(){
	fragColor = color;
	vec3 normal = passNormal;

	if ( mixTexture != 0.0)
	{
		fragColor = mix(color, texture(tex, passUVCoord), mixTexture );
	}
	if (hasNormalTex)
	{
		vec3 binormal = normalize(cross(passNormalWorld, passTangentWorld));
		mat3 tangentSpace = mat3(
			passTangentWorld.x, binormal.x, normal.x,
			passTangentWorld.y, binormal.y, normal.y,
			passTangentWorld.z, binormal.z, normal.z
		);
		normal = texture(normalTex, passUVCoord);
		normal = ( 
			(transpose(inverse(view)) * (tangentSpace * vec4(normal,0)) // turn normal according to tangent space, then bring into view space
			
		).xyz;
		normal = 

	}

    // fragPosition = vec4(passPosition,1);
    // fragUVCoord = vec4(passUVCoord,0,0);
    // fragNormal = vec4(passNormal,0);

    fragPosition = vec4(VertexOut.position,1);
    fragUVCoord = vec4(VertexOut.texCoord,0,0);
    fragNormal = vec4(VertexOut.normal,0);

}
