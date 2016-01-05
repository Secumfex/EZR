/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>

#include <Rendering/GLTools.h>
#include <Rendering/VertexArrayObjects.h>
 #include <Rendering/RenderPass.h>

// #include "UI/imgui/imgui.h"
// #include <UI/imguiTools.h>
#include <UI/Turntable.h>

// #include <Importing/TextureTools.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

static glm::vec4 s_color = glm::vec4(0.45 * 0.3f, 0.44f * 0.3f, 0.87f * 0.3f, 1.0f); // far : blueish
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);
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
	const aiScene* scene = importer.ReadFile( file, aiProcessPreset_TargetRealtime_MaxQuality);
	if (scene == NULL)
	{
		std::string errorString = importer.GetErrorString();
		DEBUGLOG->log("ERROR: " + errorString);
	} else {
		DEBUGLOG->log("Asset has been loaded successfully");
	}
	
	// print some asset info
	DEBUGLOG->log("Asset (scene) info: ");	DEBUGLOG->indent();
		DEBUGLOG->log("has meshes: "	 , scene->HasMeshes());
		DEBUGLOG->log("num meshes: "     , scene->mNumMeshes); DEBUGLOG->indent();
		for ( unsigned int i = 0; i < scene->mNumMeshes ; i++ )
		{
			aiMesh* m = scene->mMeshes[i];
			DEBUGLOG->log(std::string("mesh ") + DebugLog::to_string(i) + std::string(": ") + std::string( m->mName.C_Str() ));
		}DEBUGLOG->outdent();
	DEBUGLOG->outdent();

	DEBUGLOG->outdent();

	// create window and opengl context
	auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	/////////////////////     Scene / View Settings     //////////////////////////
	glm::vec4 eye(0.0f, 0.0f, 3.0f, 1.0f);
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
	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* m = scene->mMeshes[i];
		
		if(m->HasPositions())
		{
			// read info
			std::vector<unsigned int> indices;
			std::vector<float> vertices;
			std::vector<float> normals;
			std::vector<float> uvs;

			for ( unsigned int v = 0; v < m->mNumVertices; v++)
			{
				aiVector3D vert = m->mVertices[v];
				vertices.push_back(vert.x);
				vertices.push_back(vert.y);
				vertices.push_back(vert.z);
			}

			for ( unsigned int n = 0; n < m->mNumVertices; n++)
			{
				aiVector3D norm = m->mNormals[n];
				normals.push_back(norm.x);
				normals.push_back(norm.y);
				normals.push_back(norm.z);
			}

			for (unsigned int u = 0; u < m->mNumVertices; u++)
			{
				aiVector3D uv = m->mTextureCoords[0][u];
				uvs.push_back(uv.x);
				uvs.push_back(uv.y);
				
				if(m->GetNumUVChannels() == 3)
				{
					uvs.push_back(uv.z);
				}
			}

			for ( unsigned int f = 0; f < m->mNumFaces; f++)
			{
				aiFace face = m->mFaces[f];
				for ( unsigned int idx = 0; idx < face.mNumIndices; idx++) 
				{
					indices.push_back(face.mIndices[idx]);
				}
			}

			// method to upload data to GPU and set as vertex attribute
			auto createVbo = [](std::vector<float>& content, GLuint dimensions, GLuint attributeIndex)
			{
				GLuint vbo = 0;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, content.size() * sizeof(float), &content[0], GL_STATIC_DRAW);
				glVertexAttribPointer(attributeIndex, dimensions, GL_FLOAT, 0, 0, 0);
				glEnableVertexAttribArray(attributeIndex);
				return vbo;
			};

			auto createIndexVbo = [](std::vector<unsigned int>& content) 
			{
				GLuint vbo = 0;	
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, content.size() * sizeof(unsigned int), &content[0], GL_STATIC_DRAW);
				return vbo;
			};

			// generate VAO
			GLuint vao;
		    glGenVertexArrays(1, &vao);
			Renderable *renderable = new Renderable;
			renderable->m_vao = vao;
			glBindVertexArray(vao);

			renderable->m_positions.m_vboHandle = createVbo(vertices, 3, 0);
			renderable->m_positions.m_size = vertices.size() / 3;

			renderable->m_uvs.m_vboHandle = createVbo(uvs, (m->GetNumUVChannels() == 3) ? 3 : 2, 1);
			renderable->m_uvs.m_size = (m->GetNumUVChannels() == 3) ? uvs.size() / 3 : uvs.size() / 2;
			
			renderable->m_normals.m_vboHandle = createVbo(normals, 3, 2);
			renderable->m_normals.m_size = normals.size() / 3;

			renderable->m_indices.m_vboHandle = createIndexVbo(indices);
			renderable->m_indices.m_size = indices.size();

			renderable->setDrawMode(GL_TRIANGLES);

			glBindVertexArray(0);

			objects.push_back(renderable);
		}

	}

	
	/////////////////////// 	Renderpasses     ///////////////////////////
	 // regular GBuffer
	 DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	 ShaderProgram shaderProgram("/modelSpace/GBuffer.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	 shaderProgram.update("model", model);
	 shaderProgram.update("view", view);
	 shaderProgram.update("projection", perspective);

	 DEBUGLOG->log("FrameBufferObject Creation: GBuffer"); DEBUGLOG->indent();
	 FrameBufferObject fbo(getResolution(window).x, getResolution(window).y);
	 FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	 fbo.addColorAttachments(4); DEBUGLOG->outdent();   // G-Buffer
	 FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default

	 DEBUGLOG->log("RenderPass Creation: GBuffer"); DEBUGLOG->indent();
	 RenderPass renderPass(&shaderProgram, &fbo);
	 renderPass.addEnable(GL_DEPTH_TEST);	
	 // renderPass.addEnable(GL_BLEND);
	 renderPass.setClearColor(0.0,0.0,0.0,0.0);
	 renderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	 for (auto r : objects){renderPass.addRenderable(r);}  DEBUGLOG->outdent();

	 // regular GBuffer compositing
	 DEBUGLOG->log("Shader Compilation: GBuffer compositing"); DEBUGLOG->indent();
	 ShaderProgram compShader("/screenSpace/fullscreen.vert", "/screenSpace/finalCompositing.frag"); DEBUGLOG->outdent();
	 // set texture references
	 compShader.addTexture("colorMap", 	 fbo.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
	 compShader.addTexture("normalMap", 	 fbo.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT1));
	 compShader.addTexture("positionMap", fbo.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT2));

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
 //    ImGui_ImplGlfwGL3_Init(window, true);

	Turntable turntable;
	double old_x;
    double old_y;
	glfwGetCursorPos(window, &old_x, &old_y);
	
	// 	ImGuiIO& io = ImGui::GetIO();
	// 	if ( io.WantCaptureMouse )
	// 	{ return; } // ImGUI is handling this
	auto cursorPosCB = [&](double x, double y)
	{

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
	// 	switch (k)
	// 	{
	// 		case GLFW_KEY_W:
	// 			eye += glm::inverse(view)    * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
	// 			center += glm::inverse(view) * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
	// 			break;
	// 		case GLFW_KEY_A:
	// 			eye += glm::inverse(view)	 * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
	// 			center += glm::inverse(view) * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
	// 			break;
	// 		case GLFW_KEY_S:
	// 			eye += glm::inverse(view)    * glm::vec4(0.0f,0.0f,0.1f,0.0f);
	// 			center += glm::inverse(view) * glm::vec4(0.0f,0.0f,0.1f,0.0f);
	// 			break;
	// 		case GLFW_KEY_D:
	// 			eye += glm::inverse(view)    * glm::vec4(0.1f,0.0f,0.0f,0.0f);
	// 			center += glm::inverse(view) * glm::vec4(0.1f,0.0f,0.0f,0.0f);
	// 			break;
	// 		default:
	// 			break;
	// 	}
	// 	ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
	 };

	setCursorPosCallback(window, cursorPosCB);
	setMouseButtonCallback(window, mouseButtonCB);
	setKeyCallback(window, keyboardCB);

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
  //       ImGuiIO& io = ImGui::GetIO();
		// ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		
		// ImGui::PushItemWidth(-100);

		// ImGui::ColorEdit4( "color", glm::value_ptr( s_color)); // color mixed at max distance
  //       ImGui::SliderFloat("strength", &s_strength, 0.0f, 2.0f); // influence of color shift
        
		// ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
		// ImGui::PopItemWidth();

        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms
		shaderProgram.update( "view", view);
		shaderProgram.update( "color", s_color);
		shaderProgram.update( "model", turntable.getRotationMatrix() * model);

		compShader.update("vLightPos", view * s_lightPos);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		renderPass.render();

		compositing.render();

		// ImGui::Render();
		// glDisable(GL_BLEND);
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}