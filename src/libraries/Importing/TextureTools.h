#ifndef TEXTURE_TOOLS_H
#define TEXTURE_TOOLS_H

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace TextureTools {

	struct TextureInfo
	{
		GLuint handle;
		int width;
		int height;
		int bytesPerPixel;
	};

    GLuint loadTexture(std::string fileName, TextureInfo* texInfo = nullptr);
    GLuint loadTextureFromResourceFolder(std::string fileName, TextureInfo* texInfo = nullptr);
}

#endif