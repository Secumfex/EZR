#ifndef VERTEX_ARRAY_OBJECTS_H
#define VERTEX_ARRAY_OBJECTS_H

#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct VertexBufferObject
{
    GLuint m_vboHandle; //!< A VertexBufferObject handle
    GLuint m_size; //!< the size of a VertexBufferObject.
};

class Renderable
{

public:

    Renderable();
    ~Renderable(); 

    virtual void draw();

    inline void bind();
    inline void unbind();

    unsigned int getVertexCount(); //!< get number of vertices
    unsigned int getIndexCount();  //!< get number of indices

    void setDrawMode(GLenum type); //!< sets the mode the Renderable will be drawn with (e.g. GL_TRIANLGES)

private:

    GLuint createVbo(std::vector<float> content, GLuint dimensions, GLuint vertexAttributePointer);
	GLuint createIndexVbo(std::vector<unsigned int> content, GLuint vertexAttributePointer);

public:

    VertexBufferObject m_indices; //!< index buffer
    VertexBufferObject m_positions; //!< position buffer
    VertexBufferObject m_uvs; //!< uv buffer
    VertexBufferObject m_normals; //!< normal buffer

    GLuint m_vao; //!< VertexArrayObject handle
    GLenum m_mode; //!< the mode the Renderable will be drawn with (e.g. GL_TRIANGLES)
};


class Volume : public Renderable {
private: 
	void generateBuffers(float size_x, float size_y, float size_z);
public:
	Volume(float size = 1.0f);
	Volume(float size_x, float size_y, float size_z);

	~Volume();

	void draw() override;
};

class Quad : public Renderable {
public:
	Quad();
	
	~Quad();

	void draw() override; //!< draws the quad
};

class Terrain : public Renderable {
public:
	Terrain();
	
	~Terrain();

	void draw() override; //!< draws the terrain
};

class Sphere : public Renderable {
public:
    /**
    * @brief default Constructor
    */
    Sphere(unsigned int hSlices = 20, unsigned int vSlices = 40, float radius = 1.0f);
    
    /**
    * @brief Deconstructor
    */
    ~Sphere();

    void draw() override; //!< draws the sphere

protected:
    glm::vec3 sampleSurface(float u, float v);
};

class Grid : public Renderable {
public:
    /**
    * @brief default Constructor
    */
    Grid(unsigned int fieldsX = 10, unsigned int fieldsY = 10, float sizeX = 1.0f, float sizeY = 1.0f, bool centered = false);
    
    /**
    * @brief Deconstructor
    */
    ~Grid();

    void draw() override; //!< draws the sphere

protected:
};

#endif