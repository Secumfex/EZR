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

	virtual void drawInstanced(int numInstances);

    void bind();
    void unbind();

    unsigned int getVertexCount(); //!< get number of vertices
    unsigned int getIndexCount();  //!< get number of indices

    void setDrawMode(GLenum type); //!< sets the mode the Renderable will be drawn with (e.g. GL_TRIANLGES)

public:
	template <class T>
	static GLuint createVbo(std::vector<T> content, GLuint dimensions, GLuint vertexAttributePointer, GLenum type, bool isIntegerAttribute = false); //!< implementation at end of file

    static GLuint createVbo(std::vector<float> content, GLuint dimensions, GLuint vertexAttributePointer);
	static GLuint createIndexVbo(std::vector<unsigned int> content);

public:

    VertexBufferObject m_indices; //!< index buffer
    VertexBufferObject m_positions; //!< position buffer (vertex attribute 0)
    VertexBufferObject m_uvs; //!< uv buffer (vertex attribute 1)
    VertexBufferObject m_normals; //!< normal buffer (vertex attribute 2)
	VertexBufferObject m_tangents; //!< tangent buffer (vertex attribute 3)
	//VertexBufferObject m_bitangents; //!< bitangent buffer (vertex attribute 4)

    GLuint m_vao; //!< VertexArrayObject handle
    GLenum m_mode; //!< the mode the Renderable will be drawn with (e.g. GL_TRIANGLES)
};

class Skybox : public Renderable {
public:
	Skybox();
	~Skybox();

	void draw() override;

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

class TruncatedCone : public Renderable {
public:
    /** @brief default Constructor*/
    TruncatedCone(float height = 1.0f, float radius_bottom = 1.0f, float radius_top = 0.0f, int resolution = 20, float offset_y = 0.0f);
    
    /**@brief Deconstructor*/
    ~TruncatedCone();

    struct VertexData
    { 
        std::vector<unsigned int> indices;
        std::vector<float> positions;
        std::vector<float> uv_coords;
        std::vector<float> normals;
    };
	static VertexData generateVertexData(float height, float radius_bottom, float radius_top, int resolution, float offset_y, GLenum drawMode = GL_TRIANGLE_STRIP);


    void draw() override; //!< draws the sphere

protected:
};

template <class T>
GLuint Renderable::createVbo(std::vector<T> content, GLuint dimensions, GLuint vertexAttributePointer, GLenum type, bool isIntegerAttribute)
{
    GLuint vbo = 0;
	if ( content.size() != 0 )// && content.size() % dimensions == 0 )
	{
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, content.size() * sizeof(T), &content[0], GL_STATIC_DRAW);
		if ( isIntegerAttribute)
		{
			glVertexAttribIPointer( vertexAttributePointer, dimensions, type, 0,0); // will be left as integer values in shader
		}else
		{
	        glVertexAttribPointer(vertexAttributePointer, dimensions, type, 0, 0, 0);
		}
		glEnableVertexAttribArray(vertexAttributePointer);
	}
    return vbo;
}

#endif