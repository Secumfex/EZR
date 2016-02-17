#version 430

in vec2 passUV;
in vec3 passPosition;
in vec2 passUVCoord;
in vec3 passNormal;

uniform vec4 vLightDir;
uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out vec4 fragPosition;
layout(location = 3) out vec4 fragUVCoord;
layout(location = 4) out vec4 fragMaterial;

void main() {
   vec4 color = texture(tex,passUVCoord);

    //calculate lighting with given position, normal and lightposition
     vec3 nPosToLight = normalize( - vLightDir.xyz );

     vec3 nReflection = normalize( reflect( normalize(passPosition.xyz), normalize(passNormal.xyz) ) );

     float ambient = 0.3;
     float diffuse = max( dot(passNormal.xyz, nPosToLight), 0);
     float specular = pow( max( dot( nReflection, nPosToLight ), 0),2) * 0.25;

     fragColor = vec4(
	 color.rgb * ambient 
	 + color.rgb * diffuse 
	 + color.rgb * specular
	 , 
	 color.a);

     fragNormal = vec4(passNormal,0.0);
     fragPosition = vec4(passPosition, 1.0);
     fragUVCoord = vec4(passUVCoord,0.0,0.0);
     fragMaterial = vec4(1.0,0.0,0.0,0.0); // already lit
}