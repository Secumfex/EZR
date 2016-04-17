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
glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

static glm::vec4 s_color = glm::vec4(0.45 * 0.3f, 0.44f * 0.3f, 0.87f * 0.3f, 1.0f); // far : blueish
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);

static glm::vec3 s_scale = glm::vec3(1.0f,1.0f,1.0f);
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main()
{
	DEBUGLOG->setAutoPrint(true);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////// INIT //////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////    Import Assets (Host memory)    //////////////////////////
	DEBUGLOG->log("Setup: importing assets"); DEBUGLOG->indent();

	// get file name
	std::string input;
	DEBUGLOG->log("Please enter a valid file name located in the resources folder:");
	std::cout << RESOURCES_PATH << "/"; std::getline(cin, input);
	if (input == std::string("")) {	DEBUGLOG->log("No file name provided, loading 'cube.obj' instead."); input = "cube.obj"; }
	std::string file = RESOURCES_PATH "/" + input;

	// import using ASSIMP and check for errors
	Assimp::Importer importer;
	bool success = false;
	while (!success)
	{
		const aiScene* temp = importer.ReadFile( file, aiProcessPreset_TargetRealtime_MaxQuality);
		if (temp == NULL)
		{
			std::string errorString = importer.GetErrorString();
			DEBUGLOG->log("ERROR: " + errorString); DEBUGLOG->log("Try again...");

			std::cout << RESOURCES_PATH << "/"; std::getline(cin, input);
			if (input == std::string("")) {	DEBUGLOG->log("No file name provided, loading 'cube.obj' instead."); input = "cube.obj"; }
			file = RESOURCES_PATH "/" + input;
			importer.FreeScene();
			importer.ReadFile( file, aiProcessPreset_TargetRealtime_MaxQuality);
		}
		else
		{
			success = true;
		}
	} 
	const aiScene* scene = importer.GetScene();

	DEBUGLOG->log("Asset has been loaded successfully");
	
	// print some asset info
	DEBUGLOG->log("Asset (scene) info: ");	DEBUGLOG->indent();
		DEBUGLOG->log("has meshes: "	 , scene->HasMeshes());
		DEBUGLOG->log("num meshes: "     , scene->mNumMeshes); DEBUGLOG->indent();
		for ( unsigned int i = 0; i < scene->mNumMeshes ; i++ )
		{
			aiMesh* m = scene->mMeshes[i];
			DEBUGLOG->log(std::string("mesh ") + DebugLog::to_string(i) + std::string(": ") + std::string( m->mName.C_Str() ));
		}
		for ( unsigned int i = 0; i < scene->mNumMaterials; i++ )
		{
			DEBUGLOG->log(std::string("material ") + DebugLog::to_string(i) + std::string(": "));
			auto matInfo = AssimpTools::getMaterialInfo(scene, i);
			DEBUGLOG->indent();
				AssimpTools::printMaterialInfo(matInfo);
			DEBUGLOG->outdent();
		}
		DEBUGLOG->outdent();
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

	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 10.f);

	// object sizes
	float object_size = 1.0f;

	//objects
	std::vector<Renderable* > objects;

	//positions
	DEBUGLOG->log("Setup: model matrices"); DEBUGLOG->indent();
	glm::mat4 model = glm::mat4(1.0f); DEBUGLOG->outdent();

	/////////////////////     Upload assets (create Renderables / VAOs from data)    //////////////////////////
	DEBUGLOG->log("Setup: creating VAOs from mesh data"); DEBUGLOG->indent();
//	std::vector<AssimpTools::RenderableInfo> renderableInfoVector = AssimpTools::createSimpleRenderablesFromScene( scene );
	auto vertexData = AssimpTools::createVertexDataInstancesFromScene(scene);
	auto renderables = AssimpTools::createSimpleRenderablesFromVertexDataInstances(vertexData);
	for (auto r : renderables)
	{
		objects.push_back(r);
	}

	//glm::vec3 bbox_min(FLT_MAX);
	//glm::vec3 bbox_max(-FLT_MAX);
	//for (auto r : renderableInfoVector)
	//{
	//	objects.push_back(r.renderable); // extract list of renderables
	//	DEBUGLOG->log("corresponding aiScene mesh idx: ", r.meshIdx); DEBUGLOG->indent();
	//	DEBUGLOG->log("name      : " + r.name);
	//	DEBUGLOG->log("bbox size : " , r.boundingBox.max - r.boundingBox.min); DEBUGLOG->outdent();

		//AssimpTools::checkMax(bbox_max, r.boundingBox.max);
		//AssimpTools::checkMin(bbox_min, r.boundingBox.min);

	//}
	
	// upload textures used by mesh
	std::map<aiTextureType, GLuint> textures;
	for (int i = 0; i < scene->mNumMaterials; i++)
	{
		auto matInfo = AssimpTools::getMaterialInfo(scene, i);
		DEBUGLOG->indent();
			AssimpTools::printMaterialInfo(matInfo);
		DEBUGLOG->outdent();
		for (auto t : matInfo.texture) // load all textures used with this material
		{
			GLuint tex = TextureTools::loadTextureFromResourceFolder(t.second.relativePath);
			if (tex != -1){ textures[t.first] = tex; } // save if successfull
		}
	}

	DEBUGLOG->outdent();
	// recenter view
	//center = glm::vec4(glm::vec3(center) + 0.5f * (bbox_max - bbox_min) + bbox_min, center.w);

	/////////////////////// 	Renderpasses     ///////////////////////////
	 // regular GBuffer
	 DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	 ShaderProgram shaderProgram("/modelSpace/GBuffer.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	 shaderProgram.update("model", model);
	 shaderProgram.update("view", view);
	 shaderProgram.update("projection", perspective);

	 // check for displayable textures 
	 if (textures.find(aiTextureType_DIFFUSE) != textures.end())
	 { shaderProgram.bindTextureOnUse("tex", textures.at(aiTextureType_DIFFUSE)); shaderProgram.update("mixTexture", 1.0);}
	 if (textures.find(aiTextureType_NORMALS) != textures.end() && shaderProgram.getUniformInfoMap()->find("normalTex") != shaderProgram.getUniformInfoMap()->end()) 
	 { shaderProgram.bindTextureOnUse("normalTex", textures.at(aiTextureType_NORMALS));shaderProgram.update("hasNormalTex", true);}

	 DEBUGLOG->outdent();

	 DEBUGLOG->log("FrameBufferObject Creation: GBuffer"); DEBUGLOG->indent();
	 FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	 FrameBufferObject fbo(shaderProgram.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	 FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
	 DEBUGLOG->outdent();

	 DEBUGLOG->log("RenderPass Creation: GBuffer"); DEBUGLOG->indent();
	 RenderPass renderPass(&shaderProgram, &fbo);
	 renderPass.addEnable(GL_DEPTH_TEST);	
	 // renderPass.addEnable(GL_BLEND);
	 renderPass.setClearColor(0.0,0.0,0.0,0.0);
	 renderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	 for (auto r : objects){renderPass.addRenderable(r);}  
	 DEBUGLOG->outdent();

	 // regular GBuffer compositing
	 DEBUGLOG->log("Shader Compilation: GBuffer compositing"); DEBUGLOG->indent();
	 ShaderProgram compShader("/screenSpace/fullscreen.vert", "/screenSpace/finalCompositing.frag"); DEBUGLOG->outdent();
	 // set texture references
	 compShader.bindTextureOnUse("colorMap", 	 fbo.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
	 compShader.bindTextureOnUse("normalMap", 	 fbo.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT1));
	 compShader.bindTextureOnUse("positionMap",  fbo.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT2));

	 DEBUGLOG->log("RenderPass Creation: GBuffer Compositing"); DEBUGLOG->indent();
	 Quad quad;
	 RenderPass compositing(&compShader, 0);
	 compositing.addClearBit(GL_COLOR_BUFFER_BIT);
	 compositing.setClearColor(0.25,0.25,0.35,0.0);
	 // compositing.addEnable(GL_BLEND);
	 compositing.addDisable(GL_DEPTH_TEST);
	 compositing.addRenderable(&quad);

	//////////////////////////////////////////////////////////////////////////////
	///////////////////////    GUI / USER INPUT   ////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	// Setup ImGui binding
     ImGui_ImplGlfwGL3_Init(window, true);

	Turntable turntable;
	double old_x;
    double old_y;
	glfwGetCursorPos(window, &old_x, &old_y);
	

	auto cursorPosCB = [&](double x, double y)
	{
	 	ImGuiIO& io = ImGui::GetIO();
	 	if ( io.WantCaptureMouse )
	 	{ return; } // ImGUI is handling this

		double d_x = x - old_x;
		double d_y = y - old_y;

		if ( turntable.getDragActive() )
		{
			turntable.dragBy(d_x, d_y, view);
		}

		old_x = x;
		old_y = y;
	};

	auto mouseButtonCB = [&](int b, int a, int m)
	{
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_PRESS)
		{
			turntable.setDragActive(true);
		}
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_RELEASE)
		{
			turntable.setDragActive(false);
		}

	// 	ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
	};

	 auto keyboardCB = [&](int k, int s, int a, int m)
	 {
	 	if (a == GLFW_RELEASE) {return;} 
	 	switch (k)
	 	{
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

	auto windowResizeCB = [&](int width, int height)
	{
		OPENGLCONTEXT->updateWindowCache();
		WINDOW_RESOLUTION = glm::vec2(OPENGLCONTEXT->cacheWindowSize);
		compositing.setViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
		perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 10.f);
		shaderProgram.update("projection", perspective);
	};

	setWindowResizeCallback(window, windowResizeCB);

	// model matrices / texture update function
	// std::function<void(Renderable*)> perRenderableFunction = [&](Renderable* r){ 
	// 	static int i = 0;
	// 	shaderProgram.update("model", turntable.getRotationMatrix() * modelMatrices[i]);
	// 	shaderProgram.update("mixTexture", 0.0);

	// 	i = (i+1)%modelMatrices.size();
	// 	};
	// renderPass.setPerRenderableFunction( &perRenderableFunction );

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// RENDER LOOP /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	double elapsedTime = 0.0;
	render(window, [&](double dt)
	{
		elapsedTime += dt;
		std::string window_header = "Assimp Test - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
         ImGuiIO& io = ImGui::GetIO();
		 ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		 ImGui::SliderFloat3("strength", glm::value_ptr(s_scale), 0.0f, 10.0f);
		// ImGui::PushItemWidth(-100);

		// ImGui::ColorEdit4( "color", glm::value_ptr( s_color)); // color mixed at max distance
  //       ImGui::SliderFloat("strength", &s_strength, 0.0f, 2.0f); // influence of color shift
        
		 //ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
		 //ImGui::PopItemWidth();

        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms
		shaderProgram.update( "view", view);
		shaderProgram.update( "color", s_color);
		shaderProgram.update( "model", turntable.getRotationMatrix() * model * glm::scale(s_scale));

		compShader.update("vLightPos", view * s_lightPos);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		renderPass.render();

		compositing.render();

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}
