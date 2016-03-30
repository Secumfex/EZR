/**
 * @brief GAUSS-LAPLACE-PYRAMIDE Testing Executable
 */

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>

#include "misc.cpp"
#include <UI/Turntable.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>


const int MAX_RADIUS = 16;

void reconstructInterpolateVolume(ReconstructionBase& base1, ReconstructionBase& base2, RenderPass& rayCast, int gaussBaseLevel, std::vector<GLint>& laplaceLevels)
{
	//rayCast.getShaderProgram()->update("gaussBaseLevel", gaussBaseLevel);
	glUseProgram(rayCast.getShaderProgram()->getShaderProgramHandle());
	glUniform1iv(glGetUniformLocation(rayCast.getShaderProgram()->getShaderProgramHandle(), "laplaceLevels"), laplaceLevels.size(), &laplaceLevels[0]);
	
	rayCast.render();
}

int main()
{
	DEBUGLOG->setAutoPrint(true);
	auto window = generateWindow(1024,1024);
	Quad quad;
	//TODO load image
	TextureTools::TextureInfo image1Info;
	auto image1 = TextureTools::loadTextureFromResourceFolder("lena.png", &image1Info);
	// auto image1 = TextureTools::loadTextureFromResourceFolder("test1.png", &image1Info);
	TextureTools::TextureInfo image2Info;
	auto image2 = TextureTools::loadTextureFromResourceFolder("face.jpg", &image2Info);
	// auto image2 = TextureTools::loadTextureFromResourceFolder("test2.png", &image2Info);

	// for arbitrary texture display
	ShaderProgram sh_showTex("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTextureLod.frag");
	sh_showTex.bindTextureOnUse("tex", image1);
	float lod = 0.0f;
	sh_showTex.update("lod", lod);
	RenderPass r_showTex(&sh_showTex, 0);
	r_showTex.setViewport(0,0,512,512);
	r_showTex.addRenderable(&quad);
	r_showTex.addDisable(GL_DEPTH_TEST);
	r_showTex.render(); // render to window

	// build FBO stacks (one for gauss, one for laplace)
	PyramideFBO gaussPyramide1(image1Info.width);
	PyramideFBO laplacePyramide1(image1Info.width);
	copyFBOContent(0, gaussPyramide1.fbo[0], glm::vec2(image1Info.width,image1Info.height), glm::vec2(image1Info.width,image1Info.height), GL_COLOR_BUFFER_BIT);
	ReconstructionBase reconstructionBase1;
	reconstructionBase1.gaussPyramide = &gaussPyramide1;
	reconstructionBase1.laplacePyramide = &laplacePyramide1;

	// build second stacks (one for gauss, one for laplace)
	sh_showTex.bindTextureOnUse("tex", image2);
	r_showTex.render(); // render to window
	PyramideFBO gaussPyramide2(image2Info.width);
	PyramideFBO laplacePyramide2(image2Info.width);
	copyFBOContent(0, gaussPyramide2.fbo[0], glm::vec2(image2Info.width,image2Info.height), glm::vec2(image2Info.width,image2Info.height), GL_COLOR_BUFFER_BIT);
	ReconstructionBase reconstructionBase2;
	reconstructionBase2.gaussPyramide = &gaussPyramide2;
	reconstructionBase2.laplacePyramide = &laplacePyramide2;

	// binomial filter masks
	std::vector<std::vector<GLfloat>> binomialMasks;
	computeMasks(MAX_RADIUS * 2 + 1 , binomialMasks);

	// compile gauss pyramide shader
	int radius = 3; // initial radius
	ShaderProgram sh_gaussPyramide("/screenSpace/fullscreen.vert", "/screenSpace/binomFilterNaive.frag");
	sh_gaussPyramide.update("radius", radius);
	glUniform1fv(glGetUniformLocation(sh_gaussPyramide.getShaderProgramHandle(), "binomWeights"), binomialMasks[2 * radius].size(), &binomialMasks[2 * radius][0]);

	// compile laplace pyramide shader
	ShaderProgram sh_laplacePyramide("/screenSpace/fullscreen.vert", "/screenSpace/laplacePyramide.frag");

	// compile reconstruction shader
	ShaderProgram sh_reconstruct("/screenSpace/fullscreen.vert", "/screenSpace/reconstructGLP.frag");
	
	// compile reconstruction interpolation shader
	ShaderProgram sh_reconstructInterpolate("/screenSpace/fullscreen.vert", "/screenSpace/reconstructInterpolateGLP.frag");
	
	int gaussBaseLevel = 7;
	std::vector<GLint> laplaceLevels(laplacePyramide1.numLevels, 0);
	laplaceLevels[1] = 1;


	/////////////// Volume Rendering //////////////
	Volume volume(1.0f,1.0f,0.5f);
	glm::vec4 eye(2.0f, 2.0f, 2.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));
	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 10.f);

	// shader that will render back or front faces of the volume
	ShaderProgram uvwShader("/modelSpace/volumeMVP.vert", "/modelSpace/volumeUVW.frag");
	uvwShader.update("model", glm::mat4(1.0f));
	uvwShader.update("view", view);
	uvwShader.update("projection", perspective);
	FrameBufferObject uvwFBO(uvwShader.getOutputInfoMap(), 512,512);

	ShaderProgram raycastShader("/screenSpace/fullscreen.vert", "/modelSpace/reconstructInterpolateVolumeGLP.frag");
	// raycastShader.update("model", glm::mat4(1.0f));
	// raycastShader.update("view", view);
	// raycastShader.update("projection", perspective);

	RenderPass r_uvwCoords(&uvwShader, &uvwFBO);
	r_uvwCoords.addRenderable(&volume);
	r_uvwCoords.addDisable(GL_DEPTH_TEST);
	r_uvwCoords.addClearBit(GL_COLOR_BUFFER_BIT);
	r_uvwCoords.addEnable(GL_BLEND);

	RenderPass r_rayCast(&raycastShader, 0);
	r_rayCast.setViewport(0,512,512,512);
	r_rayCast.addRenderable(&quad);
	r_rayCast.addDisable(GL_DEPTH_TEST);

	//raycastShader.bindTextureOnUse("gaussPyramide1", reconstructionBase1.gaussPyramide->texture);
	raycastShader.bindTextureOnUse("laplacePyramide1", reconstructionBase1.laplacePyramide->texture);

	//raycastShader.bindTextureOnUse("gaussPyramide2", reconstructionBase2.gaussPyramide->texture);
	raycastShader.bindTextureOnUse("laplacePyramide2", reconstructionBase2.laplacePyramide->texture);

	raycastShader.bindTextureOnUse("uvwFront", uvwFBO.getBuffer("fragUVRCoordBack"));
	raycastShader.bindTextureOnUse("uvwBack",  uvwFBO.getBuffer("fragUVRCoordFront"));

	//imgui
    ImGui_ImplGlfwGL3_Init(window, true);

	// misc
	auto keyboardCB = [&](int k, int s, int a, int m)
	 {
	 	if (a == GLFW_RELEASE) {return;} 
	 	switch (k)
	 	{
	 		case GLFW_KEY_A:
	 			radius = std::min(std::max(0,radius - 1), MAX_RADIUS);
				DEBUGLOG->log("radius: " , radius);
				sh_gaussPyramide.use();
				glUniform1fv(glGetUniformLocation(sh_gaussPyramide.getShaderProgramHandle(), "binomWeights"), binomialMasks[2 * radius].size(), &binomialMasks[2 * radius][0]);
				sh_gaussPyramide.update("radius", radius);
	 			break;
	 		case GLFW_KEY_D:
	 			radius = std::min(std::max(0,radius + 1), MAX_RADIUS);
	 			DEBUGLOG->log("radius: " , radius);
				sh_gaussPyramide.use();
				glUniform1fv(glGetUniformLocation(sh_gaussPyramide.getShaderProgramHandle(), "binomWeights"), binomialMasks[2 * radius].size(), &binomialMasks[2 * radius][0]);
				sh_gaussPyramide.update("radius", radius);
	 			break;
	 		case GLFW_KEY_S:
	 			lod = std::min(std::max(0.0f, lod - 1.0f), (float) gaussPyramide1.numLevels - 1.0f);
	 			DEBUGLOG->log("lod: " , lod);
				sh_showTex.update("lod", lod);
	 			break;
	 		case GLFW_KEY_W:
	 			lod = std::min(std::max(0.0f, lod + 1.0f), (float) gaussPyramide1.numLevels - 1.0f);
	 			DEBUGLOG->log("lod: " , lod);
				sh_showTex.update("lod", lod);
	 			break;
	 		case GLFW_KEY_0:
	 			laplaceLevels[0] = (GLint)(!(bool)laplaceLevels[0]);
	 			break;
	 		case GLFW_KEY_1:
	 			laplaceLevels[1] = (GLint)(!(bool)laplaceLevels[1]);
	 			break;
	 		case GLFW_KEY_2:
	 			laplaceLevels[2] = (GLint)(!(bool)laplaceLevels[2]);
	 			break;
	 		case GLFW_KEY_3:
	 			laplaceLevels[3] = (GLint)(!(bool)laplaceLevels[3]);
	 			break;
	 		case GLFW_KEY_4:
	 			laplaceLevels[4] = (GLint)(!(bool)laplaceLevels[4]);
	 			break;
	 		case GLFW_KEY_5:
	 			laplaceLevels[5] = (GLint)(!(bool)laplaceLevels[5]);
	 			break;
	 		case GLFW_KEY_6:
	 			laplaceLevels[6] = (GLint)(!(bool)laplaceLevels[6]);
	 			break;
	 		case GLFW_KEY_7:
	 			laplaceLevels[7] = (GLint)(!(bool)laplaceLevels[7]);
	 			break;
	 		case GLFW_KEY_8:
	 			laplaceLevels[8] = (GLint)(!(bool)laplaceLevels[8]);
	 			break;
	 		default:
	 			break;
	 	}
		ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
	 };
	setKeyCallback(window, keyboardCB);

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

	setCursorPosCallback(window, cursorPosCB);
	setMouseButtonCallback(window, mouseButtonCB);


	// try it
	double elapsedTime = 0.0f;
	render( window, [&](double dt)
	{
		elapsedTime += dt;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::vec3  rotatedEye = glm::vec3(turntable.getRotationMatrix() * eye);  
		view = glm::lookAt(glm::vec3(rotatedEye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		uvwShader.update("view", view);

		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered

		// laplace levels
        ImGui::Checkbox("0", (bool*) &laplaceLevels[0]);
        ImGui::Checkbox("1", (bool*) &laplaceLevels[1]);
        ImGui::Checkbox("2", (bool*) &laplaceLevels[2]);
        ImGui::Checkbox("3", (bool*) &laplaceLevels[3]);
        ImGui::Checkbox("4", (bool*) &laplaceLevels[4]);
        ImGui::Checkbox("5", (bool*) &laplaceLevels[5]);
        ImGui::Checkbox("6", (bool*) &laplaceLevels[6]);
        ImGui::Checkbox("7", (bool*) &laplaceLevels[7]);
        ImGui::SliderInt("gauss base", &gaussBaseLevel, 0, gaussPyramide1.numLevels-1);

		// Build reconstruction bases
		gauss(gaussPyramide1, sh_gaussPyramide, quad);
		laplace(laplacePyramide1, gaussPyramide1, sh_laplacePyramide, quad);

		gauss(gaussPyramide2, sh_gaussPyramide, quad);
		laplace(laplacePyramide2, gaussPyramide2, sh_laplacePyramide, quad);

		// reconstruct(gaussPyramide1, laplacePyramide1, sh_reconstruct, gaussBaseLevel, laplaceLevels, quad, 0);
		float t = 0.5f + sin(elapsedTime * 0.5f) * 0.5f;
		reconstructInterpolate(reconstructionBase1, reconstructionBase2, sh_reconstructInterpolate, gaussBaseLevel, laplaceLevels, t, quad, 0);

		// show filtered texture
		r_showTex.setViewport(512, 0, 512/2, 512/2);
		sh_showTex.bindTextureOnUse("tex", gaussPyramide1.texture);
		r_showTex.render();

		r_showTex.setViewport(512, 512/2, 512/2, 512/2);
		sh_showTex.bindTextureOnUse("tex", laplacePyramide1.texture);
		r_showTex.render();

		// show filtered textures
		r_showTex.setViewport(512+512/2, 0, 512/2, 512/2);
		sh_showTex.bindTextureOnUse("tex", gaussPyramide2.texture);
		r_showTex.render();

		r_showTex.setViewport(512+512/2, 512/2, 512/2, 512/2);
		sh_showTex.bindTextureOnUse("tex", laplacePyramide2.texture);
		r_showTex.render();

		//render volume
		r_uvwCoords.render();

		r_showTex.setViewport(512, 512, 512, 512);
		// sh_showTex.bindTextureOnUse("tex", uvwFBO.getBuffer("fragUVRCoordBack"));
		sh_showTex.bindTextureOnUse("tex", uvwFBO.getBuffer("fragUVRCoordFront"));
		r_showTex.render();

		// render raycasting
		reconstructInterpolateVolume(reconstructionBase1, reconstructionBase2, r_rayCast, gaussBaseLevel, laplaceLevels);

		glViewport(0, 0, 1024, 1024);
		ImGui::Render();
		glDisable(GL_BLEND);
	});

	destroyWindow(window);

	return 0;
}