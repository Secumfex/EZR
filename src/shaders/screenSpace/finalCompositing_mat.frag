#version 430

in vec2 passUV;

uniform sampler2D colorMap;
uniform sampler2D normalMap;
uniform sampler2D positionMap;
uniform sampler2D materialMap;

uniform vec4 vLightDir;
out vec4 fragmentColor;

void main() {
    vec4 position = texture(positionMap, passUV);
    if (position.a == 0.0) { discard; }
    vec4 normal =   texture(normalMap, passUV);
    vec4 color =    texture(colorMap, passUV);
    vec4 mat   = texture(materialMap, passUV);

    if (mat.x == 0 || mat.x == 2.0) // regular lighting
    {
        //calculate lighting with given position, normal and lightposition
        // vec3 nPosToLight = normalize( vLightPos.xyz - position.xyz );
        vec3 nPosToLight = normalize( - vLightDir.xyz );

        vec3 nReflection = normalize( reflect( normalize(position.xyz), normalize(normal.xyz) ) );

        float ambient = 0.1;
        float diffuse = max( dot(normal.xyz, nPosToLight), 0);
        
        float specular = 0.0;
        if ( mat.x == 0.0)
        {
            specular = pow( max( dot( nReflection, nPosToLight ), 0), mat.y) * mat.z;
        }

        fragmentColor = vec4(
    	color.rgb * ambient 
    	+ color.rgb * diffuse 
    	+ vec3(specular)
    	, 
    	color.a);
    }
    if (mat.x == 1.0) // no lighting
    {
        fragmentColor = color;
    }
}