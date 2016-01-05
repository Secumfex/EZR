#ifndef ASSIMP_TOOLS_H
#define ASSIMP_TOOLS_H

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <glm/glm.hpp>
#include <vector>

class Renderable;

namespace AssimpTools {

	struct BoundingBox // Bounding Box in local coordinates
	{
		glm::vec3 max;
		glm::vec3 min;
	};

	struct RenderableInfo
	{
		Renderable*  renderable;
		BoundingBox  boundingBox;
		std::string  name;    // name associated with this mesh
		unsigned int meshIdx; // mesh index associated with this Renderable (to retrieve the aiMesh from the source aiScene)
	};

	/** @brief creates a Renderable for every Mesh in the scene
	 * @details Each Renderable (hopefully) has vertices, normals, uvs and an index buffer, draw mode is GL_TRIANGLES */
	std::vector<RenderableInfo > createSimpleRenderablesFromScene( const aiScene* scene ); 

	BoundingBox computeBoundingBox(const aiMesh* mesh);

} // namespace AssimpTools
#endif