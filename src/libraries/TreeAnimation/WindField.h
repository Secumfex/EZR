#ifndef WINDFIELD_H
#define WINDFIELD_H

#include <glm/glm.hpp>
#include <vector>
#include <functional>

#include <Rendering/VertexArrayObjects.h>

namespace TreeAnimation
{
class WindField
{
public:

	WindField(int width, int height); 
	~WindField();

	GLuint m_vectorTextureHandle;
	std::vector<float> m_vectorTexData;

	std::function<glm::vec3(float, float, float)> m_evaluate; 

	const int m_width;
	const int m_height;

	void updateVectorTexture(double elapsedTime);
};

} // TreeAnimation
#endif