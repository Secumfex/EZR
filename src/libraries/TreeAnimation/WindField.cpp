#include "WindField.h"

//#include <stdlib.h>
//#include <functional>
//
//#include <assimp/scene.h>
//#include <Importing/AssimpTools.h>
//#include <glm/gtx/transform.hpp>

#include <Core/DebugLog.h>
#include <Rendering/OpenGLContext.h>
TreeAnimation::WindField::WindField(int width, int height)
	: m_height(height),
	m_width(width)
{
	m_vectorTextureHandle = 0;
	
	// default evaluation function
	m_evaluate = [](float t, float offsetX, float offsetY){
		float x = sin(t + offsetX * 5.0f) * 0.49f + 0.51f;
		float y = cos(t + offsetY * 5.0f) * 0.49f + 0.51f;
		float z = 0.0f;
		return glm::vec3(x,y,z);
	};
}

TreeAnimation::WindField::~WindField()
{
	if (m_vectorTextureHandle != 0)
	{
		glDeleteTextures(1,&m_vectorTextureHandle); // delete texture
	}
}


void TreeAnimation::WindField::updateVectorTexture(double elapsedTime)
{
	// first time, create it
	if (m_vectorTextureHandle == 0)
	{
		m_vectorTexData.resize(m_width * m_height * 3, 0.0);
		//DEBUGLOG->log("VectorSize: ", m_vectorTexData.size());

		glGenTextures(1, &m_vectorTextureHandle);
		OPENGLCONTEXT->bindTexture(m_vectorTextureHandle);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, m_width, m_height);	

		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		OPENGLCONTEXT->bindTexture(0);
	}

	// evaluate vectors in vector field
	for (int i = 0; i < m_height; i++)
	{
		for (int j = 0; j < m_width; j++)
		{
			auto vector = m_evaluate( (float) elapsedTime, ((float) i / (float) m_height) , ((float) j / (float) m_width) );
			//float power = length(vector);
			//vector = glm::normalize(vector);

			m_vectorTexData[ (i * m_height * 3) + (j * 3 )]    = vector.x;
			m_vectorTexData[ (i * m_height * 3) + (j * 3 + 1)] = vector.y;
			m_vectorTexData[ (i * m_height * 3) + (j * 3 + 2)] = vector.z;
			//m_vectorTexData[ (i * m_height * 4) + (j * 4 + 3)] = pwer;
		}
	}

	// upload to texture
	OPENGLCONTEXT->bindTexture(m_vectorTextureHandle, GL_TEXTURE0);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, m_width, m_height, GL_RGB, GL_FLOAT, &m_vectorTexData[0] );
	OPENGLCONTEXT->bindTexture(0);

};
