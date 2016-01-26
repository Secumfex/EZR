//!< fragment shader
#version 430

//!< in-vars
in vec2 vert_UV;

//!< uniforms
//screen infos
uniform float screenWidth; 
uniform float screenHeight;

//camera infos
uniform float cameraFOV; 
uniform float cameraNearPlane; 
uniform float cameraFarPlane; 
uniform vec3 cameraLookAt; 
uniform vec3 cameraPosition; 


uniform int user_pixelStepSize; 
uniform float fadeYparameter; 
uniform bool toggleSSR; 
uniform bool toggleGlossy; 
uniform bool optimizedSSR; 
uniform bool experimentalSSR; 
uniform bool fadeToEdges; 
uniform mat4 ViewMatrix; 
uniform mat4 ProjectionMatrix; 
 
uniform sampler2D wsPositionTex; 
uniform sampler2D vsPositionTex; 
uniform sampler2D wsNormalTex; 
uniform sampler2D vsNormalTex; 
uniform sampler2D ReflectanceTex; 
uniform sampler2D DepthTex; 
uniform sampler2D DiffuseTex; 

//!< out-vars
out vec4 FragColor;

//!< Linearizes a depth value 
//  Source: http://www.geeks3d.com/20091216/geexlab-how-to-visualize-the-depth-buffer-in-glsl/ 
float linearizeDepth(float depth) { 
 float near = cameraNearPlane; 
 float far = cameraFarPlane; 
 float linearDepth = (2.0 * near) / (far + near - depth * (far - near)); 
 
 return linearDepth; 
} 


//!< ssr func
vec4 ScreenSpaceReflections(in vec3 vsPosition, in vec3 vsNormal, in vec3 vsReflectionVector){ 

 vec4 reflectedColor = vec4(0.0); 
 vec2 pixelsize = 1.0/vec2(screenWidth, screenHeight);
 vec4 csPosition = ProjectionMatrix * vec4(vsPosition, 1.0); 
 vec3 ndcsPosition = csPosition.xyz / csPosition.w;
 vec3 ssPosition = 0.5 * ndcsPosition + 0.5;
 
 //Project reflectedvectorintoscreenspace 
 vsReflectionVector += vsPosition;
 vec4 csReflectionVector = ProjectionMatrix * vec4(vsReflectionVector, 1.0);
 vec3 ndcsReflectionVector = csReflectionVector.xyz / csReflectionVector.w;
 vec3 ssReflectionVector = 0.5 * ndcsReflectionVector + 0.5;
 ssReflectionVector = normalize(ssReflectionVector - ssPosition);
 
 vec3 lastSamplePosition; 
 vec3 currentSamplePosition;
 float initalStep; 
 float pixelStepSize;
 
 //Ray tracing 
 initalStep = 1.0 / sampleCount; 
 ssReflectionVector = ssReflectionVector * initalStep;
 lastSamplePosition = ssPosition + ssReflectionVector; 
 currentSamplePosition = lastSamplePosition + ssReflectionVector;
  
 intsampleCount = max(int(screenHeight), int(screenWidth)); 
 intcount = 0; 
 while(count< sampleCount) { 
 
  //Out ofscreenspace = break, exemplarisch, komponentenweise durchführen (x, y, z) 
  if(currentSamplePosition < 0.0 || currentSamplePosition > 1.0){
   break; 
  }
  vec2samplingPosition = currentSamplePosition.xy; 
 
  //http://www.ozone3d.net/blogs/lab/20090206/how-to-linearize-the-depth-value/ 
  floatsampledDepth = linearizeDepth( texture(DepthTex, samplingPosition).z ); 
  floatrayDepth = linearizeDepth(currentSamplePosition.z);
  floatdelta = abs(rayDepth-sampledDepth);
 
  //Step ray 
  if(rayDepth< sampledDepth){ 
   lastSamplePosition = currentSamplePosition; 
   currentSamplePosition = lastSamplePosition + ssReflectionVector;
  } 
  else{ 
   if(rayDepth > sampledDepth){ 
    lastSamplePosition = currentSamplePosition;
    currentSamplePosition = lastSamplePosition - ssReflectionVector;
   } 
   if(delta < minimumDelta){ 
    reflectedColor = texture(DiffuseTex, samplingPosition); 
    break; 
   } 
  }           
  count++;
 } 
 returnreflectedColor;
}

//!< main
void main(void){
 //texture information from G-Buffer
 float reflectance = texture(ReflectanceTex, vert_UV).a; 
 vec3 vsPosition = texture(vsPositionTex, vert_UV).xyz; 
 vec3 vsNormal = texture(vsNormalTex, vert_UV).xyz; 
 
 //reflection vector calculation
 //view space calculations 
 vec3 vsEyeVector        = normalize(vsPosition); 
 vec3 vsReflectionVector = normalize(reflect(vsEyeVector, vsNormal));             
 
 //screen space reflections
 if(toggleSSR){ 
  vec4 color = ScreenSpaceReflections(vsPosition, vsNormal, vsReflectionVector); 
  //color = mix(color, texture(ReflectanceTex, vert_UV), 0.5); 
  FragColor = reflectance * color; 
 } 
}



//!< pseudo-code
input : Reflected ray R,
Depth buffer texture DepthT ex (G-Buffer),
Diffuse texture DiffuseT ex (lighting pass),
output: Reflected color RC
1 ssR = project R into screen space;
2 while Raymarch along ssR do
3 if ssR.z > DepthT ex at position ssR.xy) then
4 RC = DiffuseT ex at position ssR.xy);
5 end
6 end
7 return RC;