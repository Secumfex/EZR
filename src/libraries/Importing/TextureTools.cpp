#include <iostream>

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "TextureTools.h"

#include "Core/DebugLog.h"

namespace TextureTools {
	GLuint loadTexture(std::string fileName, TextureInfo* texInfo){

    	std::string fileString = std::string(fileName);
    	fileString = fileString.substr(fileString.find_last_of("/"));

    	int width, height, bytesPerPixel;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &bytesPerPixel, 0);

        if(data == NULL){
        	DEBUGLOG->log("ERROR : Unable to open image " + fileString);
        	  return -1;}

        //create new texture
        GLuint textureHandle;
        glGenTextures(1, &textureHandle);
     
        //bind the texture
        glBindTexture(GL_TEXTURE_2D, textureHandle);
     
        //send image data to the new texture
        if (bytesPerPixel < 3) {
        	DEBUGLOG->log("ERROR : Unable to open image " + fileString);
            return -1;
        } else if (bytesPerPixel == 3){
            glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        } else if (bytesPerPixel == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        } else {
        	DEBUGLOG->log("Unknown format for bytes per pixel... Changed to \"4\"");
            glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }

        //texture settings
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, true);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);
        DEBUGLOG->log( "SUCCESS: image loaded from " + fileString );

		if (texInfo != nullptr)
		{
			texInfo->bytesPerPixel = bytesPerPixel;
			texInfo->handle = textureHandle;
			texInfo->height = height;
			texInfo->width = width;
		}

        return textureHandle;
    }

	GLuint loadTextureFromResourceFolder(std::string fileName, TextureInfo* texInfo){
    	std::string filePath = RESOURCES_PATH "/" + fileName;

		return loadTexture(filePath, texInfo);
    }

	GLuint loadCubemap(std::vector<std::string> faces)
	{
	GLuint textureID;
	glGenTextures(1, &textureID);

	int width,height,bytesPerPixel;

	unsigned char* image;
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	for(GLuint i = 0; i < faces.size(); i++)
	{
		stbi_set_flip_vertically_on_load(false);
        image = stbi_load(faces[i].c_str(), &width, &height, &bytesPerPixel, 0);
		        //send image data to the new texture
        if (bytesPerPixel < 3) {
			DEBUGLOG->log("ERROR : Unable to open image " + faces[i]);
            return -1;
        } else if (bytesPerPixel == 3){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        } else if (bytesPerPixel == 4) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        } else {
        	DEBUGLOG->log("Unknown format for bytes per pixel... Changed to \"4\"");
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        }
	    stbi_image_free(image);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return textureID;
	}


	GLuint loadCubemapFromResourceFolder(std::vector<std::string> fileNames){
		for (unsigned int i = 0; i < fileNames.size(); i++)
		{
			fileNames[i] = RESOURCES_PATH "/" + fileNames[i];
		}
		return loadCubemap(fileNames);
    }
}
