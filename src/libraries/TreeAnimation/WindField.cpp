#include "WindField.h"

//#include <stdlib.h>
//#include <functional>
//
//#include <assimp/scene.h>
//#include <Importing/AssimpTools.h>
//#include <glm/gtx/transform.hpp>

#include <Core/DebugLog.h>
#include <Rendering/OpenGLContext.h>

#ifdef MINGW_THREADS
 	#include <mingw-std-threads/mingw.thread.h>
#else
 	#include <thread>
#endif
#include <atomic>

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


void TreeAnimation::WindField::createVectorTexture()
{
	m_vectorTexData.resize(m_width * m_height * 3, 0.0);
	//DEBUGLOG->log("VectorSize: ", m_vectorTexData.size());

	glGenTextures(1, &m_vectorTextureHandle);
	OPENGLCONTEXT->bindTexture(m_vectorTextureHandle);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, m_width, m_height);	

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	OPENGLCONTEXT->bindTexture(0);

}

void TreeAnimation::WindField::updateVectorTextureData(double elapsedTime)
{
	for (int i = 0; i < m_height; i++)
	{
		for (int j = 0; j < m_width; j++)
		{
			auto vector = m_evaluate( (float) elapsedTime, ((float) i / (float) m_height) , ((float) j / (float) m_width) );
			
			m_vectorTexData[ (i * m_height * 3) + (j * 3 )]    = vector.x;
			m_vectorTexData[ (i * m_height * 3) + (j * 3 + 1)] = vector.y;
			m_vectorTexData[ (i * m_height * 3) + (j * 3 + 2)] = vector.z;
		}
	}
}

void TreeAnimation::WindField::uploadVectorTextureData()
{
	// upload to texture
	OPENGLCONTEXT->bindTextureToUnit(m_vectorTextureHandle, GL_TEXTURE0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, m_width, m_height, GL_RGB, GL_FLOAT, &m_vectorTexData[0] );
	OPENGLCONTEXT->bindTexture(0);
}

// static variables used in threaded updates
static std::atomic<bool> s_finished(true);
static std::thread s_t;

// to be run from a thread
void updateVectorTextureDataAsynchronously(TreeAnimation::WindField* windfield, double elapsedTime)
{
	windfield->updateVectorTextureData(elapsedTime);
	s_finished = true;
}

void TreeAnimation::WindField::updateVectorTextureThreaded(double elapsedTime)
{
		// first time, create it
	if (m_vectorTextureHandle == 0)
	{
		createVectorTexture();
	}

	if(s_finished) // thread s_finished
	{
		if (s_t.joinable())	s_t.join();
		uploadVectorTextureData();

		s_finished = false;
		s_t = std::thread(&updateVectorTextureDataAsynchronously, this, elapsedTime);
	}
}

void TreeAnimation::WindField::updateVectorTexture(double elapsedTime)
{
	// first time, create it
	if (m_vectorTextureHandle == 0)
	{
		createVectorTexture();
	}

	// evaluate vectors in vector field
	updateVectorTextureData(elapsedTime);

	//upload
	uploadVectorTextureData();

};

TreeAnimation::WindField::~WindField()
{
	if (m_vectorTextureHandle != 0)
	{
		glDeleteTextures(1,&m_vectorTextureHandle); // delete texture
	}

	if (s_t.joinable())
	{	
		s_t.join();
	}
}
