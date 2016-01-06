/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>
//#include <random>
#include <time.h>

#include <Rendering/GLTools.h>
#include <Rendering/VertexArrayObjects.h>
#include <Rendering/RenderPass.h>

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>
#include <UI/Turntable.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <TreeAnimation/Tree.h>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

static glm::vec4 s_color = glm::vec4(0.45 * 0.3f, 0.44f * 0.3f, 0.87f * 0.3f, 1.0f); // far : blueish
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);

static float s_strength = 1.0f;
static bool  s_isRotating = false;

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main()
{
	DEBUGLOG->setAutoPrint(true);
	auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////// INIT /////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////    create Tree data    //////////////////////////
	DEBUGLOG->log("Setup: generating trees"); DEBUGLOG->indent();

	float tree_size = 4.0f; // tree size
	TreeAnimation::Tree tree(1.0f, tree_size, 1.0f);
	
	// generate a tree randomly
	srand (time(NULL));	
	auto addRandomBranch = [](TreeAnimation::Tree* tree, TreeAnimation::Tree::Branch* parent, float rPosMin, float rLengthMax, float rLengthMin, float rPitchMin, float rPitchMax)
	{
		float r1 = ((float) rand()) / ((float) RAND_MAX);
		float r2 = ((float) rand()) / ((float) RAND_MAX);
		float rLength = ( r1 * (rLengthMax - rLengthMin)) + rLengthMin;
		float rPitchAngle = ( r2 * (rPitchMax - rPitchMin) )+ rPitchMin;
		float rYawAngle = ((float) rand()) / ((float) RAND_MAX) * 2.f * glm::pi<float>();
		float rPos = (((float) rand()) / ((float) RAND_MAX) * (1.0f - rPosMin)) + rPosMin ;
		DEBUGLOG->log("rPos: ", rPos);
		
		glm::vec3 rDirection = glm::rotateY(glm::rotateZ(glm::vec3(1.0f, 0.0f, 0.0f), rPitchAngle), rYawAngle);

		return tree->addBranch(
			parent,
			rDirection,
			rPos,
			rPos * parent->thickness,
			rLength,
			TreeAnimation::Tree::computeStiffness( rPos * parent->thickness, (1.0f - rPos) * parent->thickness, rLength, tree->m_E)); 
	};

	for ( int i = 0; i < 5; i++)
	{
		auto branch = addRandomBranch(
			&tree,
			&tree.m_trunk,
			0.50f,
			tree.m_trunk.length / 2.0f,
			tree.m_trunk.length / 4.0f, 
			glm::radians( -20.0f),
			glm::radians( 20.0f));

		for ( int j = 0; j < 5; j++)
		{
			auto subBranch = addRandomBranch(
				&tree,
				branch,
				0.1f,
				branch->length / 2.0f,
				branch->length / 8.0f,
				glm::radians(-20.f),
				glm::radians(45.0f));
		}
	}

	auto printTree = [](TreeAnimation::Tree& tree)
	{
		DEBUGLOG->log("Tree");
		DEBUGLOG->indent();
		DEBUGLOG->log("num trunk branches: ", tree.m_trunk.children.size());
		for ( int i = 0; i < tree.m_trunk.children.size(); i++)
		{
			DEBUGLOG->log("branch " + DebugLog::to_string(i) + ": ");
			DEBUGLOG->indent();
			    DEBUGLOG->log("direction: ", tree.m_trunk.children[i]->direction );
				DEBUGLOG->log("length   : ", tree.m_trunk.children[i]->length);
				DEBUGLOG->log("origin   : ", tree.m_trunk.children[i]->origin);

				DEBUGLOG->log("num sub branches: ", tree.m_trunk.children[i]->children.size());
				int k = 0;
				for ( auto b : tree.m_trunk.children[i]->children)
				{
					DEBUGLOG->log("sub branch "+ DebugLog::to_string(k));
					DEBUGLOG->indent();
					DEBUGLOG->log(" direction: ", b->direction );
					DEBUGLOG->log(" length   : ", b->length);
					DEBUGLOG->log(" origin   : ", b->origin);
					DEBUGLOG->outdent();
					k++;
				}
		
			DEBUGLOG->outdent();
		}
		DEBUGLOG->outdent();
	};

	printTree(tree);

	/////////////////////    generate Tree Renderable    //////////////////////////
	auto generateRenderableFromTree = []( TreeAnimation::Tree& tree)
	{
		Renderable* renderable = new Renderable();
		
		std::vector<unsigned int> indices;
		std::vector<float> vertices;

		auto addVert = [&]( glm::vec3& vert)
		{
			vertices.push_back(vert.x);
			vertices.push_back(vert.y);
			vertices.push_back(vert.z);

			indices.push_back(indices.size());
		};

		std::function<void(TreeAnimation::Tree::Branch*)> addBranchRecursively = [&](TreeAnimation::Tree::Branch* b)
		{
			addVert(b->origin);
			glm::vec3 end = b->origin + b->direction * b->length;
			addVert(end);
			for ( auto subB : b->children)
			{
				addBranchRecursively(subB);
			}
		};

		addBranchRecursively( &tree.m_trunk );

	    glGenVertexArrays(1, &renderable->m_vao);
		glBindVertexArray(renderable->m_vao);

		renderable->m_positions.m_vboHandle = Renderable::createVbo(vertices, 3, 0);
		renderable->m_positions.m_size = vertices.size() / 3;
		renderable->m_indices.m_vboHandle = Renderable::createIndexVbo(indices);
		renderable->m_indices.m_size = indices.size();

		renderable->setDrawMode(GL_LINES);
		
		glBindVertexArray(0); 

		return renderable;
	};

	glLineWidth(5.0f); // chunky lines

	std::vector<Renderable*> objects;
	Renderable* renderable = generateRenderableFromTree(tree);
	objects.push_back(renderable);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	/////////////////////     Scene / View Settings     //////////////////////////
	glm::vec4 eye(0.0f, 0.0f, 3.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));

	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 10.f);
	
	DEBUGLOG->log("Setup: model matrices"); DEBUGLOG->indent();
	glm::mat4 model = glm::translate(glm::vec3(0.0f, - tree_size / 2.0f, 0.0f)); DEBUGLOG->outdent();

	/////////////////////// 	Renderpasses     ///////////////////////////
	 // regular GBuffer
	 DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	 ShaderProgram shaderProgram("/modelSpace/GBuffer.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	 shaderProgram.update("model", model);
	 shaderProgram.update("view", view);
	 shaderProgram.update("projection", perspective);
	 DEBUGLOG->outdent();

	 DEBUGLOG->log("FrameBufferObject Creation: GBuffer"); DEBUGLOG->indent();
	 FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	 FrameBufferObject fbo(shaderProgram.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	 FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
	 DEBUGLOG->outdent();

	 DEBUGLOG->log("RenderPass Creation: GBuffer"); DEBUGLOG->indent();
	 RenderPass renderPass(&shaderProgram, &fbo);
	 renderPass.addEnable(GL_DEPTH_TEST);	
	 renderPass.setClearColor(0.0,0.0,0.0,0.0);
	 renderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	  for (auto r : objects){renderPass.addRenderable(r);}  
	 DEBUGLOG->outdent();

	 // regular GBuffer compositing
	 DEBUGLOG->log("Shader Compilation: GBuffer compositing"); DEBUGLOG->indent();
	 ShaderProgram compShader("/screenSpace/fullscreen.vert", "/screenSpace/finalCompositing.frag"); DEBUGLOG->outdent();
	 compShader.bindTextureOnUse("colorMap", 	 fbo.getBuffer("fragColor"));
	 compShader.bindTextureOnUse("normalMap", 	 fbo.getBuffer("fragNormal"));
	 compShader.bindTextureOnUse("positionMap",  fbo.getBuffer("fragPosition"));

	 DEBUGLOG->log("RenderPass Creation: GBuffer Compositing"); DEBUGLOG->indent();
	 Quad quad;
	 RenderPass compositing(&compShader, 0);
	 compositing.addClearBit(GL_COLOR_BUFFER_BIT);
	 compositing.setClearColor(0.25,0.25,0.35,0.0);
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

		ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
	};

	 auto keyboardCB = [&](int k, int s, int a, int m)
	 {
	 	if (a == GLFW_RELEASE) {return;} 
		switch(k){
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

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// RENDER LOOP /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	double elapsedTime = 0.0;
	render(window, [&](double dt)
	{
		elapsedTime += dt;
		std::string window_header = "Tree Animation Test - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		
		ImGui::PushItemWidth(-100);

		ImGui::ColorEdit4( "color", glm::value_ptr( s_color)); // color mixed at max distance
        ImGui::SliderFloat("strength", &s_strength, 0.0f, 2.0f); // influence of color shift
        
		ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
		ImGui::PopItemWidth();

	   {
			static bool pause;
			static ImVector<float> values; if (values.empty()) { values.resize(90); memset(values.Data, 0, values.Size*sizeof(float)); } 
			static int values_offset = 0; 
			if (!pause) 
			{
				static float refresh_time = ImGui::GetTime(); // Create dummy data at fixed 60 hz rate for the demo
				for (; ImGui::GetTime() > refresh_time + 1.0f/60.0f; refresh_time += 1.0f/60.0f)
				{
					static const float pi = 3.1415926535f;
					static float x = 0.0f;
					values[values_offset] = cos(x*pi) * cos(x * 3 * pi) * cos( x * 5 * pi) * cos( x * 7 * pi) + sin (x * 25 * pi) * 0.1 ; 
					values_offset = (values_offset+1)%values.Size; 
					x += 0.10f*values_offset; 
				}
			}
			ImGui::PlotLines("##Graph", values.Data, values.Size, values_offset, "avg 0.0", -1.0f, 1.0f, ImVec2(0,80));
			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x); 
			ImGui::BeginGroup();
			ImGui::Text("Graph");
			ImGui::Checkbox("pause", &pause);
			ImGui::EndGroup();
		}

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

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}