#include "ShaderProgram.h"

#include "Core/DebugLog.h"

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

ShaderProgram::ShaderProgram(std::string vertexshader, std::string fragmentshader) 
{
    // Initially, we have zero shaders attached to the program
	m_shaderCount = 0;

	// Generate a unique Id / handle for the shader program
	// Note: We MUST have a valid rendering context before generating
	// the m_shaderProgramHandle or it causes a segfault!
	m_shaderProgramHandle = glCreateProgram();

    //Set up fragment shader
    Shader vertexShader(GL_VERTEX_SHADER);
	vertexShader.loadFromFile(SHADERS_PATH + vertexshader);
	vertexShader.compile();

 	//Set up fragment shader
	Shader fragmentShader(GL_FRAGMENT_SHADER);
	fragmentShader.loadFromFile(SHADERS_PATH + fragmentshader);
	fragmentShader.compile();

	//readOutputs(fragmentShader);

	// Set up shader program
	attachShader(vertexShader);
	attachShader(fragmentShader);
	link();

    mapShaderProperties(GL_UNIFORM, &m_uniformMap);
    mapShaderProperties(GL_PROGRAM_INPUT, &m_inputMap);
    mapShaderProperties(GL_PROGRAM_OUTPUT, &m_outputMap);

	// readUniforms();
}


ShaderProgram::ShaderProgram(std::string vertexshader, std::string fragmentshader, std::string geometryshader) 
{
    
    // Initially, we have zero shaders attached to the program
	m_shaderCount = 0;

	// Generate a unique Id / handle for the shader program
	// Note: We MUST have a valid rendering context before generating
	// the m_shaderProgramHandle or it causes a segfault!
	m_shaderProgramHandle = glCreateProgram();

    Shader vertexShader(GL_VERTEX_SHADER);
	vertexShader.loadFromFile(SHADERS_PATH + vertexshader);
	vertexShader.compile();

	//Set up fragment shader
	Shader fragmentShader(GL_FRAGMENT_SHADER);
	fragmentShader.loadFromFile(SHADERS_PATH + fragmentshader);
	fragmentShader.compile();

	//readOutputs(fragmentShader);

	Shader geometryShader(GL_GEOMETRY_SHADER);
	geometryShader.loadFromFile(SHADERS_PATH + geometryshader);
	geometryShader.compile();

	// Set up shader program
	attachShader(vertexShader);
	attachShader(fragmentShader);
	attachShader(geometryShader);
    link();

    mapShaderProperties(GL_UNIFORM, &m_uniformMap);
    mapShaderProperties(GL_PROGRAM_INPUT, &m_inputMap);
    mapShaderProperties(GL_PROGRAM_OUTPUT, &m_outputMap);
	//readUniforms();
}

ShaderProgram::~ShaderProgram()
{
	// Delete the shader program from the graphics card memory to
	// free all the resources it's been using
	glDeleteProgram(m_shaderProgramHandle);
}

GLint ShaderProgram::getShaderProgramHandle()
{
	return m_shaderProgramHandle;
}

void ShaderProgram::attachShader(Shader shader)
{	
	// Increment the number of shaders we have associated with the program
	m_shaderCount++;
	// Attach the shader to the program
	// Note: We identify the shader by its unique Id value
	glAttachShader( m_shaderProgramHandle, shader.getId());

}

void ShaderProgram::link()
{
	// If we have at least two shaders (like a vertex shader and a fragment shader)...
	if (m_shaderCount >= 2)
	{
		// Perform the linking process
		glLinkProgram(m_shaderProgramHandle);
		// Check the status
		GLint linkStatus;
		glGetProgramiv(m_shaderProgramHandle, GL_LINK_STATUS, &linkStatus);
		if (linkStatus == GL_FALSE)
		{
			DEBUGLOG->log("ERROR: Shader program linking failed.");
			printShaderProgramInfoLog();
			glfwTerminate();
		}
		else
		{
			DEBUGLOG->log("Shader program linking OK.");
		}
	}
	else
	{
		DEBUGLOG->log("Can't link shaders - you need at least 2, but attached shader count is only: " + DebugLog::to_string(m_shaderCount));
		glfwTerminate();
	}
}

void ShaderProgram::printShaderProgramInfoLog() {
    GLint logLength;
    glGetProgramiv(m_shaderProgramHandle, GL_INFO_LOG_LENGTH, &logLength);
    if(logLength > 0){
        char* log = (char*) malloc(logLength);
        GLsizei written;
        glGetProgramInfoLog(m_shaderProgramHandle, logLength, &written, log);
		DEBUGLOG->log(std::string( log ));
        free(log);
    }
}

void ShaderProgram::bindTextureOnUse(const std::string &textureName, GLuint textureHandle)
{	
	m_textureMap[textureName] = textureHandle;
}

GLuint ShaderProgram::uniform(const std::string &uniform)
{
	// Note: You could do this method with the single line:
	//
	// 		return m_uniformMap[uniform];
	//
	// But we're not doing that. Explanation in the attribute() method above
	// Create an iterator to look through our uniform map and try to find the named uniform
	std::map<std::string, Info>::iterator it = m_uniformMap.find(uniform);
	// Found it? Great - pass it back! Didn't find it? Alert user and halt.
	if ( it != m_uniformMap.end() )
	{
		return m_uniformMap[uniform].location;
	}
	else
	{
		DEBUGLOG->log("ERROR: Could not find uniform in shader program: " + uniform);
		return 0;
	}
}

GLuint ShaderProgram::buffer(const std::string &buffer)
{
	// Note: You could do this method with the single line:
	//
	// 		return m_uniformMap[uniform];
	//
	// But we're not doing that. Explanation in the attribute() method above
	// Create an iterator to look through our uniform map and try to find the named uniform
	std::map<std::string, Info>::iterator it = m_outputMap.find(buffer);
	// Found it? Great - pass it back! Didn't find it? Alert user and halt.
	if ( it != m_outputMap.end() )
	{
		return m_outputMap[buffer].location;
	}
	else
	{
		DEBUGLOG->log("ERROR: Could not find buffer in shader program: " + buffer);
		return 0;
	}
}

GLuint ShaderProgram::texture(const std::string &texture)
{
	// Note: You could do this method with the single line:
	//
	// 		return m_uniformMap[uniform];
	//
	// But we're not doing that. Explanation in the attribute() method above
	// Create an iterator to look through our uniform map and try to find the named uniform
	std::map<std::string, GLuint>::iterator it = m_textureMap.find(texture);
	// Found it? Great - pass it back! Didn't find it? Alert user and halt.
	if ( it != m_textureMap.end() )
	{
		return m_textureMap[texture];
	}
	else
	{
		DEBUGLOG->log("ERROR: Could not find texture in shader program: " +texture);
		return 0;
	}
}

ShaderProgram* ShaderProgram::update(std::string name, bool value) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform1i(uniform(name), value);
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, int value) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform1i(uniform(name), value);
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, float value) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform1f(uniform(name), value);

	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, double value) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform1f(uniform(name), value);
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const glm::ivec2& vector) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform2iv(uniform(name), 1, glm::value_ptr(vector));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const glm::ivec3& vector) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform3iv(uniform(name), 1, glm::value_ptr(vector));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name,const glm::ivec4& vector) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform4iv(uniform(name), 1, glm::value_ptr(vector));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const glm::vec2& vector) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform2fv(uniform(name), 1, glm::value_ptr(vector));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const glm::vec3& vector) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform3fv(uniform(name), 1, glm::value_ptr(vector));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const glm::vec4& vector) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform4fv(uniform(name), 1, glm::value_ptr(vector));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const glm::mat2& matrix) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniformMatrix2fv(uniform(name), 1, GL_FALSE, glm::value_ptr(matrix));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const glm::mat3& matrix) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniformMatrix3fv(uniform(name), 1, GL_FALSE, glm::value_ptr(matrix));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const glm::mat4& matrix) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniformMatrix4fv(uniform(name), 1, GL_FALSE, glm::value_ptr(matrix));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const std::vector<glm::vec2>& vector) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform2fv(uniform(name), sizeof(vector), glm::value_ptr((&vector[0])[0]));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const std::vector<glm::vec3>& vector) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform3fv(uniform(name), sizeof(vector), glm::value_ptr((&vector[0])[0]));
	return this;
}

ShaderProgram* ShaderProgram::update(std::string name, const std::vector<glm::vec4>& vector) 
{
	glUseProgram(m_shaderProgramHandle);
	glUniform4fv(uniform(name), sizeof(vector), glm::value_ptr((&vector[0])[0]));
	return this;
}

void ShaderProgram::use()
{	
	int i = 0;

	for(auto texture : m_textureMap)
	{	
		// std::cout << "name: " << texture.first << ", active texture:" << i << ", uniform: " << uniform(texture.first) << ", handle: " << texture.second << std::endl;

		update( texture.first, i );
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_2D, texture.second);
		i++;
	}

	glUseProgram(m_shaderProgramHandle);
}

void ShaderProgram::disable()
{
	glUseProgram(0);
}

void ShaderProgram::mapShaderProperties(GLenum interface, std::map<std::string, Info>* map) {
	GLint numAttrib = 0;
	glGetProgramInterfaceiv(m_shaderProgramHandle, interface, GL_ACTIVE_RESOURCES, &numAttrib);

	std::vector<GLenum> properties;
	properties.push_back(GL_NAME_LENGTH);
	properties.push_back(GL_TYPE);
	properties.push_back(GL_ARRAY_SIZE);
	properties.push_back(GL_LOCATION);
	std::vector<GLint> values(properties.size());

	for(int attrib = 0; attrib < numAttrib; ++attrib)
	{
		glGetProgramResourceiv(m_shaderProgramHandle, interface, attrib, properties.size(),
		&properties[0], values.size(), NULL, &values[0]);
		
		Info info;
		info.type = values[1];
		info.location = values[3];

		std::vector<GLchar> nameData(256);
		nameData.resize(properties[0]); //The length of the name.
		glGetProgramResourceName(m_shaderProgramHandle, interface, attrib, nameData.size(), NULL, &nameData[0]);
		std::string name = std::string((char*)&nameData[0], nameData.size() - 1);
		name = std::string(name.c_str());
		(*map)[name] = info;
	}
}

void ShaderProgram::printUniformInfo() {
	DEBUGLOG->log("Shader Program " + DebugLog::to_string( m_shaderProgramHandle) + " Uniform info:" ); DEBUGLOG->indent();
		printInfo(&m_uniformMap);
	DEBUGLOG->outdent();
}
void ShaderProgram::printInputInfo() {
	DEBUGLOG->log("Shader Program " + DebugLog::to_string( m_shaderProgramHandle) + " Input info:" ); DEBUGLOG->indent();
		printInfo(&m_inputMap);
	DEBUGLOG->outdent();
}

void ShaderProgram::printOutputInfo() {
	DEBUGLOG->log("Shader Program " + DebugLog::to_string( m_shaderProgramHandle) + " Output info:" ); DEBUGLOG->indent();
		printInfo(&m_outputMap);
	DEBUGLOG->outdent();
}

void ShaderProgram::printInfo(std::map<std::string, Info>* map) {
	for(auto e : *map) {
		DEBUGLOG->log( "name: " + e.first ); 
		DEBUGLOG->indent();
			DEBUGLOG->log( "type: " + getTypeString(e.second.type) );
			DEBUGLOG->log( "location: " + DebugLog::to_string(e.second.location));
		DEBUGLOG->outdent();
	}
}

std::string ShaderProgram::getTypeString(GLenum type) {
	switch (type) {
	case 35670: 
		return "bool";
	case 5124: 
		return "int";
	case 5125: 
		return "unsigned int";
	case 5126: 
		return "float";
	case 35667: 
		return "ivec2";
	case 35668: 
		return "ivec3";
	case 35669: 
		return "ivec4";
	case 35664: 
		return "vec2";
	case 35665: 
		return "vec3";
	case 35666: 
		return "vec4";
	case 35674: 
		return "mat2";
	case 35675: 
		return "mat3";
	case 35676: 
		return "mat4";
	case 35677: 
		return "sampler1D";
	case 35678: 
		return "sampler2D";
	case 35679: 
		return "sampler3D";
	}
	return "unknown";
	//std::to_string(type);
}