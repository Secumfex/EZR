#version 430

in vec2 passUV;
in vec3 passPosition;
in vec2 passUVCoord;
in vec3 passNormal;

uniform vec4 vLightPos;
uniform sampler2D tex;

out vec4 fragmentColor;

void main() {
   vec4 color = texture(tex,passUVCoord);

    //calculate lighting with given position, normal and lightposition
     vec3 nPosToLight = normalize( vLightPos.xyz - passPosition.xyz );

     vec3 nReflection = normalize( reflect( normalize(passPosition.xyz), normalize(passNormal.xyz) ) );

     float ambient = 0.3;
     float diffuse = max( dot(passNormal.xyz, nPosToLight), 0);
     float specular = pow( max( dot( nReflection, nPosToLight ), 0),2) * 0.25;

     fragmentColor = vec4(
	 color.rgb * ambient 
	 + color.rgb * diffuse 
	 + color.rgb * specular
	 , 
	 color.a);
}