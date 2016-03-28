/**
 * @brief GAUSS-LAPLACE-PYRAMIDE Testing Executable
 */

#include <Rendering/RenderPass.h>
#include <Importing/TextureTools.h>
#include <Rendering/VertexArrayObjects.h>

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>

#include "misc.cpp"

const int MAX_RADIUS = 16;

struct PyramideFBO
{
	int numLevels;
	std::vector<GLuint> fbo;
	GLuint texture;
	int size;
	PyramideFBO(int size)
		: size(size)
	{
		// generate texture
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size,size, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // does this do anything?
		glGenerateMipmap(GL_TEXTURE_2D);	

		// set texture filter parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // does this do anything?
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); 

		// generate FBO handles
		numLevels = (int) log_2( (float) size );
		fbo.resize(numLevels);

		glGenFramebuffers(numLevels, &fbo[0]);
		for ( int i = 0; i < numLevels; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, i);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};

void gauss(PyramideFBO& pyramide, ShaderProgram& reduceShader, Quad& quad)
{
	GLboolean depthTestEnableState = glIsEnabled(GL_DEPTH_TEST);
	if (depthTestEnableState) {glDisable(GL_DEPTH_TEST);}
	reduceShader.updateAndBindTexture("tex", 0, pyramide.texture);
	reduceShader.use();
	for (int level = 1; level < (int) pyramide.fbo.size(); level++)
	{
		int lodSize = pyramide.size / pow(2.0f, (float) level);
		glViewport(0, 0, lodSize, lodSize );
		glBindFramebuffer(GL_FRAMEBUFFER, pyramide.fbo[level]);
		reduceShader.update("level", level);
		quad.draw();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (depthTestEnableState){glEnable(GL_DEPTH_TEST);}
}

void laplace(PyramideFBO& laplacePyramide, PyramideFBO& gaussPyramide, ShaderProgram& laplaceShader, Quad& quad)
{
	GLboolean depthTestEnableState = glIsEnabled(GL_DEPTH_TEST);
	if (depthTestEnableState) {glDisable(GL_DEPTH_TEST);}
	laplaceShader.bindTextureOnUse("tex", gaussPyramide.texture);
	laplaceShader.use();
	for (int level = 0; level < (int) laplacePyramide.fbo.size() - 1; level++)
	{
		int lodSize = laplacePyramide.size / pow(2.0f, (float) level);
		glViewport(0, 0, lodSize, lodSize );

		glBindFramebuffer(GL_FRAMEBUFFER, laplacePyramide.fbo[level]);
		laplaceShader.update("level", level);
		quad.draw();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (depthTestEnableState){glEnable(GL_DEPTH_TEST);}
}

void reconstruct(PyramideFBO& gaussPyramide, PyramideFBO& laplacePyramide, ShaderProgram& reconstructionShader, int gaussBaseLevel, std::vector<GLint>& laplaceLevels, Quad& quad, GLuint targetFBO)
{
	// GLboolean depthTestEnableState = glIsEnabled(GL_DEPTH_TEST);
	// if (depthTestEnableState) {glDisable(GL_DEPTH_TEST);}

	// upload gaussBaseLevel and laplaceLevels to uniform array
	reconstructionShader.use();

	reconstructionShader.updateAndBindTexture("gaussPyramide", 0, gaussPyramide.texture);
	reconstructionShader.update("gaussBaseLevel", gaussBaseLevel);
	reconstructionShader.updateAndBindTexture("laplacePyramide",1, laplacePyramide.texture);
	glUniform1iv(glGetUniformLocation(reconstructionShader.getShaderProgramHandle(), "laplaceLevels"), laplaceLevels.size(), &laplaceLevels[0]);

	glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
	glViewport(0,0, laplacePyramide.size, laplacePyramide.size);
	quad.draw();

	// if (depthTestEnableState){glEnable(GL_DEPTH_TEST);}
}

int main()
{
	DEBUGLOG->setAutoPrint(true);
	auto window = generateWindow(1024,512);
	Quad quad;
	//TODO load image
	TextureTools::TextureInfo imageInfo;
	auto image = TextureTools::loadTextureFromResourceFolder("lena.png", &imageInfo);

	// for arbitrary texture display
	ShaderProgram sh_showTex("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTextureLod.frag");
	sh_showTex.bindTextureOnUse("tex", image);
	float lod = 0.0f;
	sh_showTex.update("lod", lod);
	RenderPass r_showTex(&sh_showTex, 0);
	r_showTex.setViewport(0,0,512,512);
	r_showTex.addRenderable(&quad);
	r_showTex.addDisable(GL_DEPTH_TEST);
	r_showTex.render(); // render to window

	// build FBO stacks (one for gauss, one for laplace)
	PyramideFBO gaussPyramide(imageInfo.width);
	PyramideFBO laplacePyramide(imageInfo.width);
	copyFBOContent(0, gaussPyramide.fbo[0], glm::vec2(imageInfo.width,imageInfo.height), glm::vec2(imageInfo.width,imageInfo.height), GL_COLOR_BUFFER_BIT);
	
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
	int gaussBaseLevel = 7;
	std::vector<GLint> laplaceLevels(laplacePyramide.numLevels, 0);
	laplaceLevels[1] = 1;

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
	 			lod = std::min(std::max(0.0f, lod - 1.0f), (float) gaussPyramide.numLevels - 1.0f);
	 			DEBUGLOG->log("lod: " , lod);
				sh_showTex.update("lod", lod);
	 			break;
	 		case GLFW_KEY_W:
	 			lod = std::min(std::max(0.0f, lod + 1.0f), (float) gaussPyramide.numLevels - 1.0f);
	 			DEBUGLOG->log("lod: " , lod);
				sh_showTex.update("lod", lod);
	 			break;
	 		default:
	 			break;
	 	}
		ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
	 };
	setKeyCallback(window, keyboardCB);

	// try it
	render( window, [&](double)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
        ImGui::SliderInt("gauss base", &gaussBaseLevel, 0, gaussPyramide.numLevels);

		gauss(gaussPyramide, sh_gaussPyramide, quad);
		laplace(laplacePyramide, gaussPyramide, sh_laplacePyramide, quad);

		reconstruct(gaussPyramide, laplacePyramide, sh_reconstruct, gaussBaseLevel, laplaceLevels, quad, 0);

		// show filtered texture
		r_showTex.setViewport(512, 0, 512/2, 512/2);
		sh_showTex.bindTextureOnUse("tex", gaussPyramide.texture);
		r_showTex.render();

		r_showTex.setViewport(512, 512/2, 512/2, 512/2);
		sh_showTex.bindTextureOnUse("tex", laplacePyramide.texture);
		r_showTex.render();

		glViewport(0,0,1024,512);
		ImGui::Render();
		glDisable(GL_BLEND);
	});

	destroyWindow(window);

	return 0;
}