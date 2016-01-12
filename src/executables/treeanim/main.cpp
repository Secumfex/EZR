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


#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
//#include <glm/gtx/vector_angle.hpp>

#include <TreeAnimation/Tree.h>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

static glm::vec4 s_color = glm::vec4(107.0f / 255.0f , 68.0f / 255.0f , 35.0f /255.0f, 1.0f); // far : brown
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);

static float s_wind_angle = 45.0f;
static glm::vec3 s_wind_direction = glm::rotateY(glm::vec3(1.0f,0.0f,0.0f), glm::radians(s_wind_angle));
static glm::mat4 s_wind_rotation = glm::mat4(1.0f);
static float s_wind_power = 1.0f;

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
	float tree_width = tree_height / 10.0f;
	float tree_stiffness = TreeAnimation::Tree::computeStiffness(tree_width, tree_width, tree_height, TreeAnimation::E_RED_OAK);
	TreeAnimation::Tree tree( tree_width, tree_height, tree_stiffness );

	// generate a tree randomly
	srand (time(NULL));	
	auto addRandomBranch = [&](TreeAnimation::Tree* tree, TreeAnimation::Tree::Branch* parent, float rPosMin, float rPosMax, float rLengthMax, float rLengthMin, float rPitchMin, float rPitchMax)
	{
		// randomization values
		float r1 = ((float) rand()) / ((float) RAND_MAX);
		float r2 = ((float) rand()) / ((float) RAND_MAX);
		float rLength = ( r1 * (rLengthMax - rLengthMin)) + rLengthMin;
		float rPitchAngle = ( r2 * (rPitchMax - rPitchMin) ) + rPitchMin; // should be between 0° and 180°
		float rYawAngle = ((float) rand()) / ((float) RAND_MAX) * 2.f * glm::pi<float>() - glm::pi<float>(); //between -180° and 180°
		// float rYawAngle = 0.0f;
		float rPos = (((float) rand()) / ((float) RAND_MAX) * (rPosMax - rPosMin)) + rPosMin ;

		// optimal: 90° to parent, assuming parent is pointing in (0,1,0) in object space
		glm::vec3 optimalDirection = glm::vec3(1.0f,0.0f,0.0f);
		glm::vec3 rDirection = glm::normalize( glm::rotateY( glm::rotateZ( optimalDirection, rPitchAngle), rYawAngle)); // apply randomization
		
		// retrieve object-space orientation of parent
		glm::vec3 parentDirection= parent->direction;
		glm::quat parentRotation = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), parent->direction);
		float thickness = (1.0f - rPos) * parent->thickness * 0.5f; 
		
		// apply parent orientation to this branch direction
		rDirection = (glm::rotate(parentRotation, rDirection));

		// add branch
		return tree->addBranch(
			parent,
			rDirection,
			rPos,
			thickness,
			rLength,
			TreeAnimation::Tree::computeStiffness( 
				(1.0f - rPos) * parent->thickness,
				(1.0f - rPos) * parent->thickness,
				rLength,
				tree->m_E)
			); ;
	};

	std::vector<TreeAnimation::Tree::Branch* > branches; // branches, indexed
	branches.push_back(&tree.m_trunk);
	
	for ( int i = 0; i < 7; i++)
	{
		auto branch = addRandomBranch(
			&tree,
			&tree.m_trunk,
			0.50f,
			0.75f,
			tree.m_trunk.length / 2.0f,
			tree.m_trunk.length / 4.0f,
			glm::radians(40.0f),
			glm::radians(50.0f));
		branches.push_back(branch);

		for ( int j = 0; j < 3; j++)
		{
			auto subBranch = addRandomBranch( 
				&tree,
				branch,
				0.1f,
				0.7f,
				branch->length / 2.0f,
				branch->length / 8.0f,
				glm::radians(40.0f),
				glm::radians(50.0f));
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
				DEBUGLOG->log("num sub branches: ", tree.m_trunk.children[i]->children.size());
				int k = 0;
				for ( auto b : tree.m_trunk.children[i]->children)
				{
					DEBUGLOG->log("sub branch "+ DebugLog::to_string(k));
					DEBUGLOG->indent();
					DEBUGLOG->log("idx: ", b->idx);
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

	auto generateRenderable = [&](TreeAnimation::Tree::Branch* branch)
	{
		//TruncatedCone::VertexData coneVertexData = TruncatedCone::generateVertexData(branch->length, branch->thickness / 2.0f, 0.0f, 5, 0.0f, GL_TRIANGLE_STRIP);

		//Renderable* renderable = new Renderable();

	 //   glGenVertexArrays(1, &renderable->m_vao);
		//glBindVertexArray(renderable->m_vao);

		//renderable->m_positions.m_vboHandle = Renderable::createVbo(coneVertexData.positions, 3, 0);
		//renderable->m_positions.m_size = coneVertexData.positions.size() / 3;

		//renderable->m_uvs.m_vboHandle = Renderable::createVbo(coneVertexData.uv_coords, 2, 1);
		//renderable->m_uvs.m_size = coneVertexData.uv_coords.size() / 2;

		//renderable->m_normals.m_vboHandle = Renderable::createVbo(coneVertexData.normals, 3, 2);
		//renderable->m_normals.m_size = coneVertexData.normals.size() / 3;

		//renderable->m_indices.m_vboHandle = Renderable::createIndexVbo(coneVertexData.indices);
		//renderable->m_indices.m_size = coneVertexData.indices.size();

		// generate regular Truncated Cone
		Renderable* renderable = new TruncatedCone( branch->length, branch->thickness / 2.0f, 0.0f, 20, 0.0f);
		renderable->bind();

		std::vector<unsigned int> hierarchy;
		auto addHierarchy = [&] (TreeAnimation::Tree::Branch* b) // wow this is ugly
		{
			// first index is always this branch's index
			hierarchy.push_back(b->idx);
		
			if ( b->parent != nullptr ){
				hierarchy.push_back(b->parent->idx); // parent branch
				if ( b->parent->parent != nullptr){
					hierarchy.push_back(0); // trunk
				}else{
					hierarchy.push_back(0); // trunk
				}
			}else{
				hierarchy.push_back(0); // trunk
				hierarchy.push_back(0); // trunk
			}};

		for ( int i = 0; i < renderable->m_positions.m_size; i++)
		{
			addHierarchy(branch); // add hierarchy once for every vertex
		}

		// add another vertex attribute which contains the tree hierarchy
		Renderable::createVbo<unsigned int>(hierarchy, 3, 4, GL_UNSIGNED_INT, true); 
		
		glBindVertexArray(0); 

		return renderable;
	};

	// generate one renderable per branch
	std::function<void(TreeAnimation::Tree::Branch*, std::vector<Renderable*>&)> generateRenderables = [&](
		TreeAnimation::Tree::Branch* branch, std::vector<Renderable*>& renderables)
	{
		renderables.push_back( generateRenderable(branch));
		for (int i = 0; i < branch->children.size(); i++)
		{
			generateRenderables(branch->children[i], renderables);
		}
	};
	
	std::vector<Renderable*> objects;
	generateRenderables(&tree.m_trunk, objects);
	
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
	 ShaderProgram shaderProgram("/treeAnim/tree.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	 shaderProgram.update("model", model);
	 shaderProgram.update("view", view);
	 shaderProgram.update("projection", perspective);
	 
	 //shaderProgram.printUniformInfo();
	 //shaderProgram.printInputInfo();
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
		ImGui::PushItemWidth(-125);
		
		ImGui::SliderFloat("windDirection", &s_wind_angle, 0.0f, 360.0f); 
		ImGui::SliderFloat("windPower", &s_wind_power, 0.0f, 4.0f); 

		static glm::vec3 angleshifts[3] ={glm::vec3(0.0),glm::vec3(0.0),glm::vec3(0.0)};
		ImGui::SliderFloat3("vAngleShiftFront", glm::value_ptr( angleshifts[0]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAngleShiftBack", glm::value_ptr( angleshifts[1]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAngleShiftSide", glm::value_ptr( angleshifts[2]), -1.0f, 1.0f);
		
		static glm::vec3 amplitudes[3] = {glm::vec3(1.0),glm::vec3(1.0),glm::vec3(1.0)};
		ImGui::SliderFloat3("vAmplitudesFront", glm::value_ptr( amplitudes[0]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAmplitudesBack", glm::value_ptr( amplitudes[1]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAmplitudesSide", glm::value_ptr( amplitudes[2]), -1.0f, 1.0f); 
		
		static glm::vec3 frequencies(1.0f);
		ImGui::SliderFloat3("fFrequencies", glm::value_ptr( frequencies), 0.0f, 3.0f); 

		static float tree_phase = 0.0f;
		ImGui::SliderFloat("treePhase", &tree_phase, 0.0, glm::two_pi<float>());
		
		ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms
		shaderProgram.update( "view",  view);
		shaderProgram.update( "color", s_color);
		shaderProgram.update( "model", turntable.getRotationMatrix() * model);

		shaderProgram.update("simTime", s_simulationTime);
		s_wind_direction = glm::rotateY(glm::vec3(1.0f,0.0f,0.0f), glm::radians(s_wind_angle));
		shaderProgram.update( "windDirection", s_wind_direction);
		
		glm::vec3 windTangent = glm::vec3(-s_wind_direction.z, s_wind_direction.y, s_wind_direction.x);
		float animatedWindPower = sin(s_simulationTime) * (s_wind_power / 2.0f) + s_wind_power / 2.0f; 
		s_wind_rotation = glm::rotate(glm::mat4(1.0f), (animatedWindPower / 2.0f), windTangent);
		shaderProgram.update( "windRotation" , s_wind_rotation); 

		shaderProgram.update("vAngleShiftFront", angleshifts[0]); //front
		shaderProgram.update("vAngleShiftBack", angleshifts[1]); //back
		shaderProgram.update("vAngleShiftSide", angleshifts[2]); //side
		
		shaderProgram.update("vAmplitudesFront", amplitudes[0]); //front
		shaderProgram.update("vAmplitudesBack", amplitudes[1]); //back
		shaderProgram.update("vAmplitudesSide", amplitudes[2]); //side
		
		shaderProgram.update("fFrequencyFront", frequencies.x); //front
		shaderProgram.update("fFrequencyBack", frequencies.y); //back
		shaderProgram.update("fFrequencySide", frequencies.z); //side

		shaderProgram.update("tree.phase", tree_phase); //front

		// upload tree uniforms
		for (int i = 0; i < branches.size(); i++)
		{
			std::string prefix = "tree.branches[" + DebugLog::to_string(i) + "].";

			shaderProgram.update(prefix + "origin", branches[i]->origin);
			
			float branch_phase = float(i) / branches.size();
			shaderProgram.update(prefix + "phase", 0.0f);
			shaderProgram.update(prefix + "pseudoInertiaFactor", 1.0f);
			
			// orientation is computed from object space direction relative to optimal branch axis
			glm::quat orientation = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), branches[i]->direction);
			glm::vec4 quatAsVec4 = glm::vec4(orientation.x, orientation.y, orientation.z, orientation.w);
			shaderProgram.update(prefix + "orientation", quatAsVec4);

			int parentIdx = 0; if ( branches[i]->parent != nullptr) {parentIdx = branches[i]->parent->idx;}
		}

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