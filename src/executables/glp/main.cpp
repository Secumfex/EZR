/**
 * @brief GAUSS-LAPLACE-PYRAMIDE Testing Executable
 */

#include <Rendering/RenderPass.h>
#include <Rendering/GLTools.h>
#include <Importing/TextureTools.h>
#include <Rendering/VertexArrayObjects.h>

#include <algorithm>

const int MAX_RADIUS = 16;

void computeMasks(int level, std::vector<std::vector<GLfloat>>& binomialMasks)
{
	binomialMasks.resize(level+1);

	binomialMasks[0].resize(1); // base level
	binomialMasks[0][0] = 1;

	for ( int i = 1; i <= level; i++ )
	{
		binomialMasks[i].resize(i + 1);

		for ( int j = 0; j < binomialMasks[i].size();j ++)
		{
			// left Value
			int left = 0;
			if (j-1 >= 0 && j-1 < binomialMasks[i-1].size())
			{
				left = binomialMasks[i-1][j-1]; 
			}
			int right = 0;
			if (j >= 0 && j < binomialMasks[i-1].size())
			{
				right = binomialMasks[i-1][j]; 
			}
			binomialMasks[i][j] = left + right;
		}
	}

	// normalize each level
	for ( int i = 1; i <= level; i++ )
	{
		GLfloat weight = 0.0f;
		for ( int j = 0; j < binomialMasks[i].size(); j++)
		{
			weight += binomialMasks[i][j];
		}
		for ( int j = 0; j < binomialMasks[i].size(); j++)
		{
			binomialMasks[i][j] /= weight;
		}
	}
}

float log_2( float n )  
{  
    return log( n ) / log( 2 );
}

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

int main()
{
	DEBUGLOG->setAutoPrint(true);
	auto window = generateWindow(1024,512);
	Quad quad;
	//TODO load image
	TextureTools::TextureInfo imageInfo;
	auto image = TextureTools::loadTextureFromResourceFolder("lena.png", &imageInfo);

	// for arbitrary texture display
	ShaderProgram sh_showTex("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTexture.frag");
	sh_showTex.bindTextureOnUse("tex", image);
	RenderPass r_showTex(&sh_showTex, 0);
	r_showTex.addRenderable(&quad);
	r_showTex.addDisable(GL_DEPTH_TEST);
	r_showTex.setViewport(0, 0, 512, 512);
	r_showTex.render(); // render to window

	//TODO build FBO stacks (one for gauss, one for laplace)
	PyramideFBO pyramide(imageInfo.width);
	copyFBOContent(0, pyramide.fbo[0], glm::vec2(imageInfo.width,imageInfo.height), glm::vec2(imageInfo.width,imageInfo.height), GL_COLOR_BUFFER_BIT);

	
	// binomial filter masks
	std::vector<std::vector<GLfloat>> binomialMasks;
	computeMasks(MAX_RADIUS * 2 + 1 , binomialMasks);

	//TODO compile gauss pyramide shader
	ShaderProgram sh_gaussPyramide("/screenSpace/fullscreen.vert", "/screenSpace/binomFilterNaive.frag");
	
	int radius = 3; // initial radius
	
	sh_gaussPyramide.update("radius", radius);
	glUniform1fv(glGetUniformLocation(sh_gaussPyramide.getShaderProgramHandle(), "binomWeights"), binomialMasks[2 * radius].size(), &binomialMasks[2 * radius][0]);
	sh_gaussPyramide.update("radius", radius);
	sh_gaussPyramide.bindTextureOnUse("tex", image);
	RenderPass r_gaussPyramide(&sh_gaussPyramide, 0);
	r_gaussPyramide.addRenderable(&quad);
	r_gaussPyramide.addDisable(GL_DEPTH_TEST);
	r_gaussPyramide.setViewport(512, 0, 512, 512);

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
	 		default:
	 			break;
	 	}
	 };
	setKeyCallback(window, keyboardCB);


	//TODO compile laplace pyramide shader


	//TODO try it
	render( window, [&](double)
	{
		sh_showTex.bindTextureOnUse("tex", pyramide.texture);
		r_showTex.render();

		r_gaussPyramide.render();
	});

	return 0;
}