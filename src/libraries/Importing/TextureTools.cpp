#include <iostream>

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "TextureTools.h"

#include "Core/DebugLog.h"
#include "Rendering/OpenGLContext.h"

#include "Rendering/GLTools.h"

namespace TextureTools {
    double log_2( double n )  
    {  
        return log( n ) / log( 2 );      // log(n)/log(2) is log_2. 
    }

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
		OPENGLCONTEXT->bindTexture(textureHandle);
     
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
		
		OPENGLCONTEXT->bindTexture(0);

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

	GLuint loadCubemap(std::vector<std::string> faces,bool generateMipMaps)
	{
	GLuint textureID;
	glGenTextures(1, &textureID);

	int width,height,bytesPerPixel;

	unsigned char* image;
	
	OPENGLCONTEXT->bindTexture(textureID, GL_TEXTURE_CUBE_MAP);
	for(GLuint i = 0; i < faces.size(); i++)
	{
		stbi_set_flip_vertically_on_load(false);
        image = stbi_load(faces[i].c_str(), &width, &height, &bytesPerPixel, 0);
		        //send image data to the new texture
        if (bytesPerPixel < 3) {
			DEBUGLOG->log("ERROR : Unable to open image " + faces[i]);
            return -1;
        } else if (bytesPerPixel == 3){
            // if (generateMipMaps)
            // {
            //     int numMipmaps = (int) log_2( (float) std::max(width,height) );
            //     glTexStorage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, numMipmaps, GL_RGB, width, height);  
            //     glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0,0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
            //     // glGenerateMipmap(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
            // }
            // else
            // {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
                // glGenerateMipmap(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);

            // }

        } else if (bytesPerPixel == 4) {
            // if (generateMipMaps)
            // {
            //     int numMipmaps = (int) log_2( (float) std::max(width,height) );
            //     glTexStorage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, numMipmaps, GL_RGBA, width, height);  
            //     glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0,0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);
            // }
            // else
            // {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
                // glGenerateMipmap(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
            // }
        } else {
            DEBUGLOG->log("Unknown format for bytes per pixel... Changed to \"4\"");

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        }
        stbi_image_free(image);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if(generateMipMaps)
    {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        checkGLError(true);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        checkGLError(true);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    OPENGLCONTEXT->bindTexture(0, GL_TEXTURE_CUBE_MAP);


    return textureID;
    }


    GLuint loadCubemapFromResourceFolder(std::vector<std::string> fileNames, bool generateMipMaps){
        for (unsigned int i = 0; i < fileNames.size(); i++)
        {
            fileNames[i] = RESOURCES_PATH "/" + fileNames[i];
        }
        return loadCubemap(fileNames, generateMipMaps);
    }

    GLuint loadDefaultCubemap(bool generateMipMaps)
    {
        std::vector<std::string> cubeMapFiles;
        cubeMapFiles.push_back("cubemap/cloudtop_rt.tga");
        cubeMapFiles.push_back("cubemap/cloudtop_lf.tga");
        cubeMapFiles.push_back("cubemap/cloudtop_up.tga");
        cubeMapFiles.push_back("cubemap/cloudtop_dn.tga");
        cubeMapFiles.push_back("cubemap/cloudtop_bk.tga");
        cubeMapFiles.push_back("cubemap/cloudtop_ft.tga");
        return TextureTools::loadCubemapFromResourceFolder(cubeMapFiles, generateMipMaps);
    }
}
