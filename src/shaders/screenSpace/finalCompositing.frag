#version 430

in vec2 passUV;

uniform sampler2D colorMap;
uniform sampler2D normalMap;
uniform sampler2D positionMap;

uniform vec4 vLightPos;
// uniform float strength; 
out vec4 fragmentColor;

// const int stepSize = 1;


void main() {
    vec4 position = texture(positionMap, passUV);
    if (position.a == 0.0) { discard; }
    vec4 normal =   texture(normalMap, passUV);
    vec4 color =    texture(colorMap, passUV);
    
    //calculate lighting with given position, normal and lightposition
    vec3 nPosToLight = normalize( vLightPos.xyz - position.xyz );

    vec3 nReflection = normalize( reflect( normalize(position.xyz), normalize(normal.xyz) ) );

    float ambient = 0.1;
    float diffuse = max( dot(normal.xyz, nPosToLight), 0);
    float specular = pow( max( dot( nReflection, nPosToLight ), 0),15);

    // vec4 glow = textureLod(colorMap, passUV, strength);
    // glow /= 1.0;

    fragmentColor = vec4(
	color.rgb * ambient 
	+ color.rgb * diffuse 
	+ vec3(specular)
	, 
	color.a);
    
    // fragmentColor = fragmentColor * 0.0000001 +glow;
}