#ifndef TEXTURE_TOOLS_H
#define TEXTURE_TOOLS_H

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace TextureTools {
	/** \brief !docu pls!
 	 *
 	 * @param fileName
 	 * @return GLuint
 	 */
    GLuint loadTexture(std::string fileName);
}

#endif