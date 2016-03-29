
#include <Rendering/GLTools.h>
#include <Rendering/RenderPass.h>

#include <Importing/TextureTools.h>
#include <Rendering/VertexArrayObjects.h>
#include <algorithm>

inline float log_2( float n )  
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

inline void gauss(PyramideFBO& pyramide, ShaderProgram& reduceShader, Quad& quad)
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

inline void laplace(PyramideFBO& laplacePyramide, PyramideFBO& gaussPyramide, ShaderProgram& laplaceShader, Quad& quad)
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

inline void reconstruct(PyramideFBO& gaussPyramide, PyramideFBO& laplacePyramide, ShaderProgram& reconstructionShader, int gaussBaseLevel, std::vector<GLint>& laplaceLevels, Quad& quad, GLuint targetFBO)
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

struct ReconstructionBase
{
	PyramideFBO* gaussPyramide;
	PyramideFBO* laplacePyramide;
};

inline void reconstructInterpolate(ReconstructionBase& base1, ReconstructionBase& base2, ShaderProgram& reconstructionShader, int gaussBaseLevel, std::vector<GLint>& laplaceLevels, float t, Quad& quad, GLuint targetFBO)
{
	// upload gaussBaseLevel and laplaceLevels to uniform array
	reconstructionShader.use();

	reconstructionShader.updateAndBindTexture("gaussPyramide1", 0, base1.gaussPyramide->texture);
	reconstructionShader.updateAndBindTexture("laplacePyramide1",1, base1.laplacePyramide->texture);

	reconstructionShader.updateAndBindTexture("gaussPyramide2", 2, base2.gaussPyramide->texture);
	reconstructionShader.updateAndBindTexture("laplacePyramide2", 3, base2.laplacePyramide->texture);

	reconstructionShader.update("gaussBaseLevel", gaussBaseLevel);
	reconstructionShader.update("t", t);
	glUniform1iv(glGetUniformLocation(reconstructionShader.getShaderProgramHandle(), "laplaceLevels"), laplaceLevels.size(), &laplaceLevels[0]);

	glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
	glViewport(0,0, base1.laplacePyramide->size, base1.laplacePyramide->size);
	quad.draw();
}

inline void computeMasks(int level, std::vector<std::vector<GLfloat>>& binomialMasks)
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