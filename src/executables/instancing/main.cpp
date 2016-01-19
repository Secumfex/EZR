/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>
#include <time.h>

#include <Rendering/GLTools.h>
#include <Rendering/VertexArrayObjects.h>
#include <Rendering/RenderPass.h>

//#include "UI/imgui/imgui.h"
//#include <UI/imguiTools.h>
#include <UI/Turntable.h>

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <Importing/AssimpTools.h>
#include <Importing/TextureTools.h>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

const int NUM_INSTANCES = 3000;

static glm::vec4 s_lightPos = glm::vec4(0.0,50.0f,0.0,1.0);

static float s_strength = 1.0f;
static bool  s_isRotating = false;

static std::map<Renderable*, glm::vec4*> s_renderable_color_map;
static std::map<Renderable*, int> s_renderable_material_map; //!< mapping a renderable to a material index
static std::vector<std::map<aiTextureType, GLuint>> s_material_texture_handles; //!< mapping material texture types to texture handles

//////////////////// MISC /////////////////////////////////////
float randFloat(float min, float max) //!< returns a random number between min and max
{
	return (((float) rand() / (float) RAND_MAX) * (max - min) + min); 
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


std::vector<glm::mat4 > generateModels(int numObjects, float xSize, float zSize)
{
	std::vector<glm::mat4> models(numObjects);

	for (int i = 0; i < models.size(); i++)
	{

		// generate random position on x/z plane
		float x = randFloat(-xSize * 0.5f, xSize * 0.5f);
		float z = randFloat(-zSize * 0.5f, zSize * 0.5f);
		float y = randFloat(-5.0f, 5.0f);
		models[i] = glm::scale(glm::vec3(0.1,0.1,0.1)); 
		models[i] = glm::translate(glm::vec3(x, y, z)) * models[i];
	}
	return models;
}


int main()
{
	DEBUGLOG->setAutoPrint(true);
	auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////// INIT /////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////    load tree related assets   //////////////////////////
	DEBUGLOG->log("Setup: generating renderables"); DEBUGLOG->indent();

	// import using ASSIMP and check for errors
	Assimp::Importer importer;
	DEBUGLOG->log("Loading model");
	std::string modelFile = "cube.dae";
	
	const aiScene* scene = AssimpTools::importAssetFromResourceFolder(modelFile, importer);
	auto renderable = AssimpTools::createSimpleRenderablesFromScene(scene);
	std::map<aiTextureType, AssimpTools::MaterialTextureInfo> texturesInfo;
	if (renderable.empty()) { DEBUGLOG->log("ERROR: no renderable. Going to Exit."); float wait; cin >> wait; exit(-1);}
	if (scene != NULL) texturesInfo = AssimpTools::getMaterialTexturesInfo(scene, 0);
	if (scene != NULL) s_material_texture_handles.resize(scene->mNumMaterials);

	for (auto e : texturesInfo)
	{
		GLuint texHandle = TextureTools::loadTextureFromResourceFolder(texturesInfo[e.first].relativePath);
		if (texHandle != -1){ s_material_texture_handles[e.second.matIdx][e.first] = texHandle; }
	}

	/////////////////////    create wind field          //////////////////////////
	// TreeAnimation::WindField windField(64,64);
	// windField.updateVectorTexture(0.0f);

	DEBUGLOG->outdent();
	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	/////////////////////     Scene / View Settings     //////////////////////////
	glm::vec4 eye(10.0f, 10.0f, 10.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));

	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.5f, 100.f);
	
	DEBUGLOG->log("Setup: generating model matrices"); DEBUGLOG->indent();
	srand (time(NULL));	
	std::vector<glm::mat4 > model = generateModels(NUM_INSTANCES, -15.0f, 15.0f);
	DEBUGLOG->outdent();

	/////////////////////   Instancing Settings        //////////////////////////
	DEBUGLOG->log("Setup: buffering model matrices"); DEBUGLOG->indent();
	// buffer model matrices
	renderable[0].renderable->bind();

    // Vertex Buffer Object
    //GLuint instanceModelBufferHandle;
    //glGenBuffers(1, &instanceModelBufferHandle);
    //glBindBuffer(GL_ARRAY_BUFFER, instanceModelBufferHandle);
    //glBufferData(GL_ARRAY_BUFFER, NUM_INSTANCES * sizeof(glm::mat4), &model[0], GL_STATIC_DRAW);
	 GLuint instanceModelBufferHandle = bufferData<glm::mat4>(model, GL_STATIC_DRAW);
    
    // mat4 Vertex Attribute == 4 x vec4 attributes (consecutively)
	GLuint instancedAttributeLocation = 4; // beginning attribute location (0..3 are reserved for pos,uv,norm,tangents
    GLsizei vec4Size = sizeof(glm::vec4);
    glEnableVertexAttribArray(instancedAttributeLocation); 
    glVertexAttribPointer(instancedAttributeLocation, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (GLvoid*)0); // offset = 0 x vec4 size , stride = 4x vec4 size
    glEnableVertexAttribArray(instancedAttributeLocation+1); 
    glVertexAttribPointer(instancedAttributeLocation+1, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (GLvoid*)(vec4Size)); //offset = 1 x vec4 size
    glEnableVertexAttribArray(instancedAttributeLocation+2); 
    glVertexAttribPointer(instancedAttributeLocation+2, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (GLvoid*)(2 * vec4Size)); //offset = 2 x vec4 size
    glEnableVertexAttribArray(instancedAttributeLocation+3); 
    glVertexAttribPointer(instancedAttributeLocation+3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (GLvoid*)(3 * vec4Size)); // offset = 2x vec4 size

	// enable instanced attribute processing
    glVertexAttribDivisor(instancedAttributeLocation,   1);
    glVertexAttribDivisor(instancedAttributeLocation+1, 1);
    glVertexAttribDivisor(instancedAttributeLocation+2, 1);
    glVertexAttribDivisor(instancedAttributeLocation+3, 1);
	renderable[0].renderable->unbind();
	DEBUGLOG->outdent();

	/////////////////////// 	Shader     ///////////////////////////
	// simple shader
	DEBUGLOG->log("Shader Compilation: instanced simple lighting"); DEBUGLOG->indent();
	ShaderProgram shaderProgram("/modelSpace/instancedModelViewProjection.vert", "/modelSpace/simpleLighting.frag"); DEBUGLOG->outdent();
	shaderProgram.update("view", view);
	shaderProgram.update("projection", perspective);
	shaderProgram.update("color", glm::vec4(0.7,0.7,0.7,1.0));
	DEBUGLOG->outdent();

	
	//////////////////////////////////////////////////////////////////////////////
	///////////////////////    GUI / USER INPUT   ////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	// Setup ImGui binding
    //ImGui_ImplGlfwGL3_Init(window, true);

	Turntable turntable;
	double old_x;
    double old_y;
	glfwGetCursorPos(window, &old_x, &old_y);
	
	auto cursorPosCB = [&](double x, double y)
	{
		//ImGuiIO& io = ImGui::GetIO();
		//if ( io.WantCaptureMouse )
		//{ return; } // ImGUI is handling this

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

		//ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
	};

	setCursorPosCallback(window, cursorPosCB);
	setMouseButtonCallback(window, mouseButtonCB);

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// RENDER LOOP /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	double elapsedTime = 0.0;
	render(window, [&](double dt)
	{
		elapsedTime += dt;
		std::string window_header = "Instancing Test - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
		// ImGuiIO& io = ImGui::GetIO();
		// ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered	
		// ImGui::PushItemWidth(-125);

		// ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		glm::vec3  rotatedEye = glm::vec3(turntable.getRotationMatrix() * eye);  
		view = glm::lookAt(glm::vec3(rotatedEye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		shaderProgram.update( "view",  view);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		// clear stuff
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// bind VAO
		renderable[0].renderable->bind();

		// activate shader
		shaderProgram.use();

		// render instanced
		if (renderable[0].renderable->m_indices.m_size != 0) // indices have been provided, use these
		{
			glDrawElementsInstanced( renderable[0].renderable->m_mode, renderable[0].renderable->m_indices.m_size, GL_UNSIGNED_INT, 0, NUM_INSTANCES );
		}
		else // no index buffer has been provided, lets assume this has to be rendered in vertex order
		{
			glDrawArraysInstanced(renderable[0].renderable->m_mode, 0, renderable[0].renderable->m_positions.m_size, NUM_INSTANCES );
		}
		glBindVertexArray(0);
	
		// ImGui::Render();
		// glDisable(GL_BLEND);
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}