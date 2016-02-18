//ssr fragment shader
#version 430

//in-vars
in vec2 vert_UV;

//outvars
out vec4 FragColor;

//uniforms
uniform float screenWidth; 
uniform float screenHeight;
uniform float camNearPlane; 
uniform float camFarPlane; 
uniform int user_pixelStepSize; 
uniform bool fadeToEdges;
uniform mat4 projection; 
uniform mat4 view;

uniform sampler2D vsPositionTex; 
uniform sampler2D vsNormalTex; 
uniform sampler2D ReflectanceTex; 
uniform sampler2D DepthTex; 
uniform sampler2D DiffuseTex; 
uniform samplerCube CubeMapTex;

//for otimized ssr
const float pi = 3.14159265f;
const vec2 sampleOffsets[16] = vec2[](
    vec2(-0.3857104f, -0.8171502f),
    vec2(0.03329741f, -0.9189221f),
    vec2(-0.8200077f, -0.5234867f),
    vec2(-0.07335605f, -0.2632172f),
    vec2(0.6341613f, -0.7207248f),
    vec2(-0.9161543f, -0.1177677f),
    vec2(0.9068835f, -0.2405542f),
    vec2(0.4021704f, -0.1239841f),
    vec2(-0.4632118f, 0.058327f),
    vec2(0.09567857f, 0.2931803f),
    vec2(0.8622051f, 0.1983728f),
    vec2(0.5084426f, 0.623606f),
    vec2(0.2892076f, -0.5152128f),
    vec2(-0.8960632f, 0.4230295f),
    vec2(-0.4352404f, 0.8662457f),
    vec2(-0.03035933f, 0.9384609f)
);


//!< Linearizes a depth value 
//  Source: http://www.geeks3d.com/20091216/geexlab-how-to-visualize-the-depth-buffer-in-glsl/ 
float linearizeDepth(float depth) { 
	float near = camNearPlane; 
	float far = camFarPlane; 
 	float linearDepth = (2.0 * near) / (far + near - depth * (far - near)); 
 
	return linearDepth; 
} 

//ssr function
vec4 ScreenSpaceReflections(in vec3 vsPosition, in vec3 vsNormal, in vec3 vsReflectionVector){
    
    float factor = dot(vsReflectionVector, vsNormal);

    //vars
    vec4 reflectedColor = vec4(0.0);
    vec2 pixelsize = 1.0/vec2(screenWidth, screenHeight);

    //get texture informations
    vec4 csPosition = projection * vec4(vsPosition, 1.0);
    vec3 ndcsPosition = csPosition.xyz / csPosition.w;
    vec3 ssPosition = 0.5 * ndcsPosition + 0.5;

    //project reflected vector into screen space
    vsReflectionVector += vsPosition;
    vec4 csReflectionVector = projection * vec4(vsReflectionVector, 1.0);
    vec3 ndcsReflectionVector = csReflectionVector.xyz / csReflectionVector.w;
    vec3 ssReflectionVector = 0.5 * ndcsReflectionVector + 0.5;
    ssReflectionVector = normalize(ssReflectionVector - ssPosition);

    vec3 lastSamplePosition;
    vec3 currentSamplePosition;

    float initalStep;
    float pixelStepSize;
    
    // Ray trace
    initalStep = 1.0/screenHeight;
    ssReflectionVector = normalize(ssReflectionVector) * initalStep;

    lastSamplePosition = ssPosition + ssReflectionVector;
    currentSamplePosition = lastSamplePosition + ssReflectionVector;

    float bigStep = 12 * factor;
    int sampleCount = max(int(screenHeight), int(screenWidth))/int(bigStep);
    int count = 0;

    while(count < sampleCount) {
    	//control loops
        if(count > 150){
	//		reflectedColor = texture(CubeMapTex, vsReflectionVector);	
			break;
		}
        // Out of screen space --> break
        if(currentSamplePosition.x < 0.0 || currentSamplePosition.x > 1.0 ||
           currentSamplePosition.y < 0.0 || currentSamplePosition.y > 1.0 ||
           currentSamplePosition.z < 0.0 || currentSamplePosition.z > 1.0)
        {
       		break;
        }
            
        vec2 samplingPosition = currentSamplePosition.xy;
        float sampledDepth = linearizeDepth( texture(DepthTex, samplingPosition).z );
                  
        float currentDepth = linearizeDepth(currentSamplePosition.z);
            
        float delta = abs(currentDepth - sampledDepth);

        // Step ray
        if(currentDepth < sampledDepth){
            lastSamplePosition = currentSamplePosition;
            currentSamplePosition = lastSamplePosition + ssReflectionVector * bigStep;
        }
        else{
        	if(currentDepth > sampledDepth){
 				for(float i=0; i < bigStep; i += 1.0 * factor){
                       lastSamplePosition = currentSamplePosition;
                       currentSamplePosition = lastSamplePosition - ssReflectionVector * 0.05 * i * factor;
                }
         	}
            if(delta < 0.015){
               bool toggleGlossy = true;	//da noch nicht als uniform
               if(toggleGlossy){
					float diskSize = 0.00001f;

                    reflectedColor = texture2D(DiffuseTex, samplingPosition);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 0] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 1] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 2] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 3] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 4] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 5] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 6] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 7] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 8] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[ 9] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[10] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[11] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[12] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[13] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[14] * diskSize);
                    reflectedColor += texture2D(DiffuseTex, samplingPosition + sampleOffsets[15] * diskSize);

                    reflectedColor /= 17;
               }
				else {
                	reflectedColor = texture2D(DiffuseTex, samplingPosition);
                }

                break;
 			}
		}                    
		count++;
	}

	bool fadeToEdges = true; 	//da noch nicht als uniform
    //fading to screen edges
    vec2 fadeToScreenEdge = vec2(1.0);
    if(fadeToEdges){
        fadeToScreenEdge.x = distance(lastSamplePosition.x , 1.0);
        fadeToScreenEdge.x *= distance(lastSamplePosition.x, 0.0) * 4.0;
        fadeToScreenEdge.y = distance(lastSamplePosition.y, 1.0);
        fadeToScreenEdge.y *= distance(lastSamplePosition.y, 0.0) * 4.0;
    }
    return (reflectedColor * fadeToScreenEdge.x * fadeToScreenEdge.y);
}

//cubemap function
vec4 CubeMapping(in vec3 wsReflectionVector){
	vec4 reflectedColor = vec4(0.0);
	vec3 temp = texture(CubeMapTex, wsReflectionVector).rgb;
	reflectedColor = vec4(temp,1.0f);	
	return reflectedColor;
}


//main
void main(void){

 float test1 = texture(ReflectanceTex, vert_UV).x;
 if(test1 == 2.0f){	
 	float reflectance = texture(ReflectanceTex, vert_UV).z;
 
	vec3 vsPosition = texture(vsPositionTex, vert_UV).xyz; 

 	vec3 vsNormal = texture(vsNormalTex, vert_UV).xyz; 
 
 	//reflection vector calculation
 	//view space calculations 
 	vec3 vsEyeVector        = normalize(vsPosition); 
 	vec3 vsReflectionVector = normalize(reflect(vsEyeVector, vsNormal));        
 
 	vec3 wsReflectionVector = inverse(mat3(view)) * vsReflectionVector;
 
 	//screen space reflections
  	vec4 color = ScreenSpaceReflections(vsPosition, vsNormal, vsReflectionVector); 
 
 	if(color.x == 0.0f && color.y == 0.0f && color.z == 0.0f && color.a == 0.0f){	
 		color=CubeMapping(wsReflectionVector);
 	} 
  	FragColor = color; //* reflectance
	}
  	else {
  		vec4 color = texture(DiffuseTex, vert_UV);
  		FragColor = color;
  	}
}