/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>
#include <Rendering/GLTools.h>
#include <Rendering/RenderPass.h>
#include <Rendering/VertexArrayObjects.h>
 
#ifdef MINGW_THREADS
 	#include <mingw-std-threads/mingw.thread.h>
#else
 	#include <thread>
#endif
#include <atomic>

////////////////////// PARAMETERS /////////////////////////////
const int NUM_THREADS = 10;

void* call_from_thread(int tid) 
{
	std::cout << "Launched from thread" << tid <<std::endl;
	return NULL;
}

static const int VECTOR_TEXTURE_SIZE = 512;

static GLuint s_vectorTexture = 0;
static GLuint s_texHandle = 0;

static std::vector<float> s_vectorTexData;

const glm::vec2 WINDOW_RESOLUTION = glm::vec2( VECTOR_TEXTURE_SIZE, VECTOR_TEXTURE_SIZE);

void createVectorTexture()
{
	s_vectorTexData.resize(VECTOR_TEXTURE_SIZE*VECTOR_TEXTURE_SIZE*3, 0.0);
	glGenTextures(1, &s_vectorTexture);
	OPENGLCONTEXT->bindTexture(s_vectorTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, VECTOR_TEXTURE_SIZE, VECTOR_TEXTURE_SIZE);	
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	OPENGLCONTEXT->bindTexture(0);
}

void uploadVectorTextureData()
{
	OPENGLCONTEXT->bindTexture(s_vectorTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, VECTOR_TEXTURE_SIZE, VECTOR_TEXTURE_SIZE, GL_RGB, GL_FLOAT, &s_vectorTexData[0] );
	OPENGLCONTEXT->bindTexture(0);
}

// to be run from a thread
void updateVectorTextureDataAsynchronously(double elapsedTime, std::atomic<bool>* finished )
{
	auto evaluate = [&](float t, float offsetX, float offsetY){
		float x = sin(t + offsetX) * 0.5f + 0.5f;
		float y = cos(t + offsetY) * 0.5f + 0.5f;
		float z = 0.0f;
		return glm::vec3(x,y,z);
	};

	for (int i = 0; i < VECTOR_TEXTURE_SIZE; i++)
	{
		for (int j = 0; j < VECTOR_TEXTURE_SIZE; j++)
		{
			auto vector = evaluate( (float) elapsedTime, ((float) i / (float) VECTOR_TEXTURE_SIZE) * 5.0f , ((float) j / (float) VECTOR_TEXTURE_SIZE) * 5.0f );
			s_vectorTexData[ (i * VECTOR_TEXTURE_SIZE * 3) + (j * 3 )] = vector.x;
			s_vectorTexData[ (i * VECTOR_TEXTURE_SIZE * 3) + (j * 3 + 1)] = vector.y;
			s_vectorTexData[ (i * VECTOR_TEXTURE_SIZE * 3) + (j * 3 + 2)] = vector.z;
		}
	}

	*finished = true;
}



int main()
{
	auto window = generateWindow(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y, 200, 200);
	DEBUGLOG->setAutoPrint(true);
    
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// multithreading /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	createVectorTexture(); // setup texture handle
	std::thread t;
	std::atomic<bool> finished(false);
	
	// show texture
	Quad quad;
	ShaderProgram showTexShader("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTexture.frag");
	RenderPass showTex(&showTexShader,0);
	showTex.addRenderable(&quad);
	showTex.addClearBit(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	showTex.setViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
	showTexShader.bindTextureOnUse("tex", s_vectorTexture);

	///////////////////////////////////////////////////////////////////////////////

	int loopsSinceLastUpdate = 0;
	double elapsedTime = 0.0;
	t = std::thread(&updateVectorTextureDataAsynchronously, 0.0, &finished);
	while (!shouldClose(window))
	{	
		// do stuff
		double dt = elapsedTime;
		elapsedTime = glfwGetTime();
		dt = elapsedTime - dt;
		glfwSetWindowTitle(window, DebugLog::to_string( 1.0 / dt ).c_str());

		// asynchronously do stuff with the vector texture data
		if (finished)
		{
			t.join();
			uploadVectorTextureData();

			DEBUGLOG->log("loops since last update: ", loopsSinceLastUpdate);
			DEBUGLOG->log("update time: ", elapsedTime);
			loopsSinceLastUpdate = 0;
			finished = false;
			t = std::thread(&updateVectorTextureDataAsynchronously, elapsedTime, &finished);
		}		
		loopsSinceLastUpdate++;

		// render with current texture
		showTex.render();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// in case it was still running when the window was closed
	t.join();

	return 0;
}