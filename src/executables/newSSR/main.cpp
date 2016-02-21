/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <Importing/AssimpTools.h>

#include <Rendering/GLTools.h>
#include <Rendering/VertexArrayObjects.h>
#include <Rendering/RenderPass.h>

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>
#include <UI/Turntable.h>

#include <Importing/TextureTools.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(1024.0f, 768.0f);
static glm::vec4 s_color = glm::vec4(0.45 * 0.3f, 0.44f * 0.3f, 0.87f * 0.3f, 1.0f); // far : blueish
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);
static glm::vec3 s_scale = glm::vec3(1.0f,1.0f,1.0f);

float cameraNear = 0.1f;
float cameraFar = 200.0f;	//200
int rayStep = 0.0f;
//bool fadeEdges = true;	//in shader festgelegt

static std::map<Renderable*, int> rendMatMap; 						//mapping a renderable to a material index
static std::vector<std::map<aiTextureType, GLuint>> matTexHandles; 	//mapping material texture types to texture handles

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int main(){
	std::cout<<"new main"<<endl;
	DEBUGLOG->setAutoPrint(true);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////// INIT //////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////    Import Assets (Host memory)    //////////////////////////
	DEBUGLOG->log("Setup: importing assets"); DEBUGLOG->indent();

	// get file name
	std::string input = "tscene3.obj";
	std::string file = RESOURCES_PATH "/" + input;
	// import using ASSIMP and check for errors
	Assimp::Importer importer;
	bool success = false;

	const aiScene* temp = importer.ReadFile( file, aiProcessPreset_TargetRealtime_MaxQuality);
	const aiScene* scene = importer.GetScene();
	//const aiScene* scene = AssimpTools::importAssetFromResourceFolder(input, importer);

	if (scene != NULL) matTexHandles.resize(scene->mNumMaterials);	//resize material texture map vector

	DEBUGLOG->log("Asset has been loaded successfully");
	DEBUGLOG->outdent();
	DEBUGLOG->log("Asset (scene) info: ");	DEBUGLOG->indent();
		DEBUGLOG->log("has meshes: "	 , scene->HasMeshes());
		DEBUGLOG->log("num meshes: "     , scene->mNumMeshes); DEBUGLOG->indent();
		for ( unsigned int i = 0; i < scene->mNumMeshes ; i++ ){
			aiMesh* m = scene->mMeshes[i];
			DEBUGLOG->log(std::string("mesh ") + DebugLog::to_string(i) + std::string(": ") + std::string( m->mName.C_Str() ));
		}
		for ( unsigned int i = 0; i < scene->mNumMaterials; i++ ){
			DEBUGLOG->log(std::string("material ") + DebugLog::to_string(i) + std::string(": "));
			auto matInfo = AssimpTools::getMaterialInfo(scene, i);
			DEBUGLOG->indent();
				AssimpTools::printMaterialInfo(matInfo);
			DEBUGLOG->outdent();
		}
		DEBUGLOG->outdent();
	DEBUGLOG->outdent();

	// create window and opengl context
	auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////     Scene / View Settings     //////////////////////////
	glm::vec4 eye(0.0f, 0.0f, 5.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));
	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, cameraFar);

	// object sizes
	float object_size = 1.0f;

	//objects
	std::vector<Renderable* > objects;

	//positions
	DEBUGLOG->log("Setup: model matrices"); DEBUGLOG->indent();
	glm::mat4 model = glm::mat4(1.0f); DEBUGLOG->outdent();

	/////////////////////     Upload assets (create Renderables / VAOs from data)    //////////////////////////
	DEBUGLOG->log("Setup: creating VAOs from mesh data"); DEBUGLOG->indent();
	//std::vector<AssimpTools::RenderableInfo> renderableInfoVector = AssimpTools::createSimpleRenderablesFromScene( scene );
	auto vertexData = AssimpTools::createVertexDataInstancesFromScene(scene);
	auto renderables = AssimpTools::createSimpleRenderablesFromVertexDataInstances(vertexData);
	int n = 0;
	for (auto r : renderables){
		objects.push_back(r);
		//fill rendable material map
		int mi = scene->mMeshes[n]->mMaterialIndex;
		rendMatMap.insert ( std::pair<Renderable*,int>(r,mi) );
		n=n+1;
	}
	std::cout << "sizeOBJ: " << objects.size() << endl;
	std::cout << "sizeREN: " << renderables.size() << endl;
	std::cout << "sizeRMM: " << rendMatMap.size() << endl;

	//pro mesh
	for(int j = 0; j < scene->mNumMeshes; j++){
		std::map<aiTextureType, GLuint> textures;
		auto matInfo = AssimpTools::getMaterialInfo(scene, j);
		DEBUGLOG->indent();
		AssimpTools::printMaterialInfo(matInfo);
		DEBUGLOG->outdent();
		for (auto t : matInfo.texture){		// load all textures used with this material
			GLuint tex = TextureTools::loadTextureFromResourceFolder(t.second.relativePath);
			if (tex != -1){ textures[t.first] = tex; } // save if successfull
		}
		matTexHandles.push_back(textures);

	//std::cout << textures.size() << endl;		//for debugging
	}
	//std::cout << "sizeMTH: " << matTexHandles.size() << endl;		//for debugging
	DEBUGLOG->outdent();

	/////////////////////// 	Renderpasses     ///////////////////////////

	//gbuffer pass
	DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	ShaderProgram gShader("/screenSpaceReflection/gBuffer2.vert", "/screenSpaceReflection/gBuffer2.frag"); DEBUGLOG->outdent();
	//gShader.update
	gShader.update("model",model);
	gShader.update("view",view);
	gShader.update("projection",perspective);
	//gShader.update("lightColor",s_color);
	gShader.update("color",s_color);
	//gShader.bindTextureOnUse

	DEBUGLOG->outdent();

	//gbuffer fbo
	DEBUGLOG->log("FrameBufferObject Creation: GBuffer"); DEBUGLOG->indent();
	FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	FrameBufferObject gFBO(gShader.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
	DEBUGLOG->outdent();

	DEBUGLOG->log("RenderPass Creation: GBuffer"); DEBUGLOG->indent();
	RenderPass gPass(&gShader, &gFBO);
	gPass.addEnable(GL_DEPTH_TEST);
	//renderPass.addEnable(GL_BLEND);
	gPass.setClearColor(0.0,0.0,0.0,0.0);
	gPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	for (auto r : objects){gPass.addRenderable(r);}
	DEBUGLOG->outdent();

//!!light pass recently not included!!
	 //light pass
   /*DEBUGLOG->log("Shader Compilation: light"); DEBUGLOG->indent();
	 ShaderProgram lightShader("/screenSpaceReflection/light.vert", "/screenSpaceReflection/light.frag"); DEBUGLOG->outdent();
 	 */
	 //lightShader.update
	 //lightShader.bindTextureOnUse

	 //light fbo

	 //DEBUGLOG->log("RenderPass Creation: light"); DEBUGLOG->indent();
	 Quad quad;
	 //RenderPass lightPass(&lightShader, &lightFBO);

	 //screen space render pass
	 DEBUGLOG->log("Shader Compilation: ssrShader"); DEBUGLOG->indent();
	 ShaderProgram ssrShader("/screenSpaceReflection/screenSpaceReflection.vert", "/screenSpaceReflection/screenSpaceReflection2.frag"); DEBUGLOG->outdent();
	 //ssrShader.update
	 ssrShader.update("screenWidth",getResolution(window).x);
	 ssrShader.update("screenHeight",getResolution(window).y);
	 ssrShader.update("camNearPlane",cameraNear);
	 ssrShader.update("camFarPlane",cameraFar);
	 ssrShader.update("user_pixelStepSize",rayStep);
	 //ssrShader.update("fadeToEdges",fadeEdges);	//in shader festgelegt
	 ssrShader.update("projection",perspective);
	 //ssrShader.bindTextureOnUse
	 ssrShader.bindTextureOnUse("vsPositionTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT2));
	 ssrShader.bindTextureOnUse("vsNormalTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT1));
	 ssrShader.bindTextureOnUse("ReflectanceTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT4));
	 ssrShader.bindTextureOnUse("DepthTex",gFBO.getDepthTextureHandle());
//	 ssrShader.bindTextureOnUse("DiffuseTex",lightFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));	//!light pass recently not included
	 ssrShader.bindTextureOnUse("DiffuseTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));

	 //fbo
	 DEBUGLOG->log("FrameBufferObject Creation: ssrFBO"); DEBUGLOG->indent();
	 FrameBufferObject ssrFBO(ssrShader.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	 DEBUGLOG->outdent();

	 //render pass
	 DEBUGLOG->log("RenderPass Creation: ssrPass"); DEBUGLOG->indent();
	 RenderPass ssrPass(&ssrShader, &ssrFBO);
	 //ssrPass.addEnable(GL_DEPTH_TEST);
	 //ssrPass.addEnable(GL_BLEND);
	 ssrPass.setClearColor(0.0,0.0,0.0,0.0);
	 ssrPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	 ssrPass.addRenderable(&quad);
	 DEBUGLOG->outdent();

	 //composition
	 DEBUGLOG->log("Shader Compilation: compositing"); DEBUGLOG->indent();
	 ShaderProgram compoShader("/screenSpaceReflection/comp.vert", "/screenSpaceReflection/comp2.frag"); DEBUGLOG->outdent();
	 //compoShader.update
	 compoShader.update("NearPlane",cameraNear);
	 compoShader.update("camFarPlane",cameraFar);
	 compoShader.update("textureID;",0);
	 //compoShader.bindTextureOnUse
	 compoShader.bindTextureOnUse("vsPositionTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT2));
	 compoShader.bindTextureOnUse("vsNormalTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT1));
	 compoShader.bindTextureOnUse("ColorTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
	 compoShader.bindTextureOnUse("ReflectanceTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT4));
	 compoShader.bindTextureOnUse("MaterialTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT5));
	 compoShader.bindTextureOnUse("DepthTex",gFBO.getDepthTextureHandle());
//	 compoShader.bindTextureOnUse("DiffuseTex",lightFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));		//!light pass recently not included
	 compoShader.bindTextureOnUse("SSRTex",ssrFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));

	 DEBUGLOG->log("RenderPass Creation: Compositing"); DEBUGLOG->indent();
	 RenderPass compoPass(&compoShader, 0);
	 compoPass.addClearBit(GL_COLOR_BUFFER_BIT);
	 compoPass.setClearColor(0.25,0.25,0.35,0.0);
	 //compoPass.addEnable(GL_BLEND);
	 compoPass.addDisable(GL_DEPTH_TEST);
	 compoPass.addRenderable(&quad);

	//////////////////////////////////////////////////////////////////////////////
	///////////////////////    GUI / USER INPUT   ////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	// Setup ImGui binding
	ImGui_ImplGlfwGL3_Init(window, true);

	Turntable turntable;
	double old_x;
	double old_y;
	glfwGetCursorPos(window, &old_x, &old_y);

	auto cursorPosCB = [&](double x, double y){
		ImGuiIO& io = ImGui::GetIO();
		if ( io.WantCaptureMouse ){ return; } // ImGUI is handling this

		double d_x = x - old_x;
		double d_y = y - old_y;

		if ( turntable.getDragActive() ){
			turntable.dragBy(d_x, d_y, view);
		}

		old_x = x;
		old_y = y;
	};

	auto mouseButtonCB = [&](int b, int a, int m){
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_PRESS){
			turntable.setDragActive(true);
		}
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_RELEASE){
			turntable.setDragActive(false);
		}

	// 	ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
	};

	auto keyboardCB = [&](int k, int s, int a, int m){
		 if (a == GLFW_RELEASE) {return;}
		 switch (k){
		 	case GLFW_KEY_W:
		 		eye += glm::inverse(view)    * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
		 		center += glm::inverse(view) * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
		 		break;
		 	case GLFW_KEY_A:
		 		eye += glm::inverse(view)	 * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
		 		center += glm::inverse(view) * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
		 		break;
		 	case GLFW_KEY_S:
		 		eye += glm::inverse(view)    * glm::vec4(0.0f,0.0f,0.1f,0.0f);
		 		center += glm::inverse(view) * glm::vec4(0.0f,0.0f,0.1f,0.0f);
		 		break;
		 	case GLFW_KEY_D:
		 		eye += glm::inverse(view)    * glm::vec4(0.1f,0.0f,0.0f,0.0f);
		 		center += glm::inverse(view) * glm::vec4(0.1f,0.0f,0.0f,0.0f);
		 		break;
		 	default:
		 		break;
		 }
		 ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
	 };

	setCursorPosCallback(window, cursorPosCB);
	setMouseButtonCallback(window, mouseButtonCB);
	setKeyCallback(window, keyboardCB);

	//uniform update pro object/mesh
	std::function<void(Renderable*)> perRenderableFunction = [&](Renderable* r){
	 	//gShader.update("model", turntable.getRotationMatrix() * modelMatrices[i]);	//using only one modelmatrix
		//gShader.update("color",col);													//color pro object/mesh?!

	 	int k = rendMatMap.find(r)->second;
	 	//std::cout << "k: " << k << endl;		//for debugging
	 	if (! matTexHandles.empty()){
	 		auto difftex = matTexHandles[k].find(aiTextureType_DIFFUSE);
	 		if ( difftex != matTexHandles[k].end()){
	 			gShader.bindTextureOnUse("ColorTex", difftex->second);
	 			gShader.update("mixTexture", 1.0f);
	 		}
	 		auto normaltex = matTexHandles[k].find(aiTextureType_NORMALS);
	 		if (normaltex != matTexHandles[k].end()){
	 			gShader.bindTextureOnUse("NormalTex", normaltex->second);
	 			gShader.update("hasNormalTex", true);
	 		}
	 	}
	 	float mId = scene->mMeshes[k]->mMaterialIndex / 100.0f;
		float matRefl = 0.0f;
		//shinines out of material
		auto matInfo = AssimpTools::getMaterialInfo(scene, mId);
		if(matInfo.scalar.find(AssimpTools::ScalarType::SHININESS) != matInfo.scalar.end()){
			matRefl = matInfo.scalar[AssimpTools::ScalarType::SHININESS];
		}
		//std::cout << "matid: " << mId <<endl;		//for debugging
		gShader.update("matId",mId);
		gShader.update("matReflectance",matRefl);
	};
	gPass.setPerRenderableFunction( &perRenderableFunction );

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// RENDER LOOP /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	double elapsedTime = 0.0;
	render(window, [&](double dt){
		elapsedTime += dt;
		std::string window_header = "SSR Test - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		ImGui::SliderFloat3("strength", glm::value_ptr(s_scale), 0.0f, 10.0f);

		const char* listbox_items[] = { "Compositing", "gBuffer-Positions", "gBuffer-Normals", "gBuffer-Color", "gBuffer-MatIDs", "gBuffer-Depth", "gBuffer-Reflectance", "SSR", "Light" };
		static int listbox_item_current = 0;
		ImGui::ListBox("renderTex", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 4);

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////

		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms

		gShader.update("view", view);
		//gShader.update("lightColor", s_color);
		gShader.update("color", s_color);
		gShader.update("model", turntable.getRotationMatrix() * model * glm::scale(s_scale));

		//light pass recently not used
//		lightShader.update("view", view);
//		lightShader.update("lightPosition",view * s_lightPos);

		//update ssr uniforms

		//update compo uniforms
		compoShader.update("textureID",listbox_item_current);
		//std::cout << listbox_item_current << endl;		//for debugging

		//////////////////////////////////////////////////////////////////////////////

		////////////////////////////////  RENDERING //// /////////////////////////////
		gPass.render();
//		lightPass.render();		//light pass recently not used
		ssrPass.render();
		compoPass.render();

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}
