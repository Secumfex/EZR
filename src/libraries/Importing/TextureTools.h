#ifndef TEXTURE_TOOLS_H
#define TEXTURE_TOOLS_H

#include <string>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace TextureTools {

	/** @brief Struct that saves info about a texture*/
	struct TextureInfo
	{
		GLuint handle; //!< the OpenGL handle
		int width; //!< the texture's width in pixels
		int height;//!< the texture's height in pixels
		int bytesPerPixel; //!< the texture's amount of bytes saved per pixel, i.e. 3 for GL_RGB or 4 for GL_RGBA
	};

	/**@brief load an image file and upload it to the GPU, returning the texture's handle
	 * @param fileName of the image file to be read
	 * @param texInfo (optional) pointer to a TextureInfo instance which will be filled with the texture's properties
	 * @return the texture's texture handle or -1 (that is, the maximum unsigned int, since it's unsigned)*/
    GLuint loadTexture(std::string fileName, TextureInfo* texInfo = nullptr);
    GLuint loadTextureFromResourceFolder(std::string fileName, TextureInfo* texInfo = nullptr); //!< like above, but the file name given relative to RESOURCES_PATH, e.g. "cubeTexture.jpg" 

	/* Loads a cubemap texture from 6 individual texture faces
	// Order should be:
	// +X (right)
	// -X (left)
	// +Y (top)
	// -Y (bottom)
	// +Z (front)
	// -Z (back)
	*/
	GLuint loadCubemap(std::vector<std::string> faces);
	
	// +X (right), -X(left), +Y (top), -Y (bottom), +Z (front), -Z (back)
	GLuint loadCubemapFromResourceFolder(std::vector<std::string> fileNames) ;
}

#endif