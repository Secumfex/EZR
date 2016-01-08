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
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <TreeAnimation/Tree.h>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

static glm::vec4 s_color = glm::vec4(0.45 * 0.3f, 0.44f * 0.3f, 0.87f * 0.3f, 1.0f); // far : blueish
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);

static glm::vec3 s_wind_direction = glm::normalize(glm::vec3(1.0f, 0.0f, -1.0f));

static float s_strength = 1.0f;
static bool  s_isRotating = false;

static float s_simulationTime = 0.0f;

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

	float tree_height = 4.0f;
	float tree_width = 1.0f;
	float tree_stiffness = TreeAnimation::Tree::computeStiffness(tree_width, tree_width, tree_height, TreeAnimation::E_RED_OAK);
	TreeAnimation::Tree tree( tree_width, tree_height, tree_stiffness );
	
	std::function<glm::quat(TreeAnimation::Tree::Branch*)> getRotationRecursively = [&](TreeAnimation::Tree::Branch* branch)
	{
		glm::quat quat = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), branch->direction);
		
		//DEBUGLOG->log("quat    : ", glm::vec4(quat.w, quat.x,quat.y,quat.z));
		//DEBUGLOG->log("applied : ", glm::rotate(quat, glm::vec4(0.0f,1.0f,0.0f,0.0)));

		if ( branch->parent != nullptr )
		{
			return quat * getRotationRecursively(branch->parent);
		}

		return quat;
	};

	// generate a tree randomly
	srand (time(NULL));	
	auto addRandomBranch = [&](TreeAnimation::Tree* tree, TreeAnimation::Tree::Branch* parent, float rPosMin, float rLengthMax, float rLengthMin, float rPitchMin, float rPitchMax)
	{
		float r1 = ((float) rand()) / ((float) RAND_MAX);
		float r2 = ((float) rand()) / ((float) RAND_MAX);
		float rLength = ( r1 * (rLengthMax - rLengthMin)) + rLengthMin;
		float rPitchAngle = ( r2 * (rPitchMax - rPitchMin) )+ rPitchMin;
		float rYawAngle = ((float) rand()) / ((float) RAND_MAX) * 2.f * glm::pi<float>() - glm::pi<float>();
		float rPos = (((float) rand()) / ((float) RAND_MAX) * (1.0f - rPosMin)) + rPosMin ;

		glm::vec3 rDirection = glm::normalize(glm::rotateY(glm::rotateZ(glm::vec3(1.0f, 0.0f, 0.0f), rPitchAngle), rYawAngle));

		//glm::quat tempQuat = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), rDirection);
		//DEBUGLOG->log("dir     : ", rDirection);
		//DEBUGLOG->log("tempQuat: ", glm::vec4(tempQuat.w, tempQuat.x,tempQuat.y,tempQuat.z));
		//DEBUGLOG->log("tempRot : ", glm::rotate(tempQuat, glm::vec4(0.0f,1.0f,0.0f,0.0)));

		glm::quat parentRotation = getRotationRecursively(parent);
		
		rDirection = glm::rotate(parentRotation, rDirection); // rotate direction by parent rotation

		return tree->addBranch(
			parent,
			rDirection,
			rPos,
			rPos * parent->thickness,
			rLength,
			TreeAnimation::Tree::computeStiffness( 
				rPos * parent->thickness,
				(1.0f - rPos) * parent->thickness,
				rLength,
				tree->m_E)
			); ;
	};

	std::vector<TreeAnimation::Tree::Branch* > branches; // branches, indexed
	branches.push_back(&tree.m_trunk);
	
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
		branches.push_back(branch);

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
				branches.push_back(subBranch);
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
			    DEBUGLOG->log("idx: ", tree.m_trunk.children[i]->idx );
			 //   DEBUGLOG->log("direction: ", tree.m_trunk.children[i]->direction );
				//DEBUGLOG->log("length   : ", tree.m_trunk.children[i]->length);
				//DEBUGLOG->log("origin   : ", tree.m_trunk.children[i]->origin);

				DEBUGLOG->log("num sub branches: ", tree.m_trunk.children[i]->children.size());
				int k = 0;
				for ( auto b : tree.m_trunk.children[i]->children)
				{
					DEBUGLOG->log("sub branch "+ DebugLog::to_string(k));
					DEBUGLOG->indent();
					DEBUGLOG->log("idx: ", b->idx);
					//DEBUGLOG->log("direction: ", b->direction );
					//DEBUGLOG->log("length   : ", b->length);
					//DEBUGLOG->log("origin   : ", b->origin);
					DEBUGLOG->outdent();
					k++;
				}
		
			DEBUGLOG->outdent();
		}
		DEBUGLOG->outdent();
	};

	printTree(tree);
	
	DEBUGLOG->outdent();
	/////////////////////    generate Tree Renderable    //////////////////////////
	DEBUGLOG->log("Setup: generating renderables"); DEBUGLOG->indent();

	auto generateRenderableFromTree = []( TreeAnimation::Tree& tree)
	{
		Renderable* renderable = new Renderable();
		
		std::vector<unsigned int> indices;
		std::vector<float> vertices;
		std::vector<unsigned int> hierarchy;

		auto addVert = [&]( glm::vec3& vert)
		{
			vertices.push_back(vert.x);
			vertices.push_back(vert.y);
			vertices.push_back(vert.z);

			indices.push_back(indices.size());
		};

		auto addHierarchy = [&] (TreeAnimation::Tree::Branch* b) // wow this is ugly
		{
			// first index is always this branch's index
			hierarchy.push_back(b->idx);
		
			if ( b->parent != nullptr )
			{
				hierarchy.push_back(b->parent->idx); // parent branch
				if ( b->parent->parent != nullptr)
				{
					hierarchy.push_back(0); // trunk
				}
				else
				{
					hierarchy.push_back(0); // trunk
				}
			}
			else
			{
				hierarchy.push_back(0); // trunk
				hierarchy.push_back(0); // trunk
			}
		};

		std::function<void(TreeAnimation::Tree::Branch*)> addBranchRecursively = [&](TreeAnimation::Tree::Branch* b)
		{
			addVert(b->origin);
			glm::vec3 end = b->origin + b->direction * b->length;
			addVert(end);

			// add hierarchy twice (once for each vertex)
			addHierarchy(b);
			addHierarchy(b);

			//DEBUGLOG->log("h0 :", hierarchy[ hierarchy.size() - 6 ]);
			//DEBUGLOG->log("h1 :", hierarchy[ hierarchy.size() - 5 ]);
			//DEBUGLOG->log("h2 :", hierarchy[ hierarchy.size() - 4 ]);
			//DEBUGLOG->log("h0 :", hierarchy[ hierarchy.size() - 3 ]);
			//DEBUGLOG->log("h1 :", hierarchy[ hierarchy.size() - 2 ]);
			//DEBUGLOG->log("h2 :", hierarchy[ hierarchy.size() - 1 ]);

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

		// add another vertex attribute which contains the tree hierarchy
		Renderable::createVbo<unsigned int>(hierarchy, 3, 4, GL_UNSIGNED_INT, true); 

		//glPointSize(5.0f);
		//renderable->setDrawMode(GL_POINTS);

		renderable->setDrawMode(GL_LINES);
		
		glBindVertexArray(0); 

		return renderable;
	};

	glLineWidth(5.0f); // chunky lines

	std::vector<Renderable*> objects;
	Renderable* renderable = generateRenderableFromTree(tree);
	//Renderable* renderable = new TruncatedCone( 1.0f, 1.0f, 0.2f);
	objects.push_back(renderable);
	
	DEBUGLOG->outdent();
	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	/////////////////////     Scene / View Settings     //////////////////////////
	glm::vec4 eye(0.0f, 0.0f, 3.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));

	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 10.f);
	
	DEBUGLOG->log("Setup: model matrices"); DEBUGLOG->indent();
	glm::mat4 model = glm::translate(glm::vec3(0.0f, - tree_height / 2.0f, 0.0f)); 
	//glm::mat4 model = glm::mat4(1.0f);
	DEBUGLOG->outdent();

	/////////////////////// 	Renderpasses     ///////////////////////////
	 // regular GBuffer
	 DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	 //ShaderProgram shaderProgram("/modelSpace/GBuffer.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	 ShaderProgram shaderProgram("/treeAnim/tree.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	 shaderProgram.update("model", model);
	 shaderProgram.update("view", view);
	 shaderProgram.update("projection", perspective);
	 
	 shaderProgram.printUniformInfo();
	 shaderProgram.printInputInfo();
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
		s_simulationTime = elapsedTime;
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
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms
		shaderProgram.update( "view", view);
		shaderProgram.update( "color", s_color);
		shaderProgram.update( "model", turntable.getRotationMatrix() * model);

		//shaderProgram.update("simTime", s_simulationTime);
		//shaderProgram.update("simTimeWithDelay", s_simulationTime);
		//shaderProgram.update("strength", s_strength);

		// upload tree uniforms
		for (int i = 0; i < branches.size(); i++)
		{
			std::string prefix = "tree.branches[" + DebugLog::to_string(i) + "].";

			//shaderProgram.update(prefix + "origin", branches[i]->origin);
			glm::quat orientation = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), branches[i]->direction);
			glm::vec4 quatAsVec4 = glm::vec4(orientation.x, orientation.y, orientation.z, orientation.w);

			shaderProgram.update(prefix + "orientation", quatAsVec4);
			//glm::vec3 tangent = glm::vec3(1.0,0.0,0.0);
			//if (branches[i]->parent != nullptr)
			//{
			//	tangent = glm::normalize(glm::cross(
			//		branches[i]->direction,
			//		branches[i]->parent->direction));
			//}
			//shaderProgram.update(prefix + "tangent", tangent); //TODO
			//shaderProgram.update(prefix + "stiffness", branches[i]->stiffness);
			int parentIdx = 0; if ( branches[i]->parent != nullptr) {parentIdx = branches[i]->parent->idx;}
			shaderProgram.update(prefix + "parentIdx", parentIdx);
		}

		
		//shaderProgram.update( "windDirection", s_wind_direction);

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