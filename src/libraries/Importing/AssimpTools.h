#ifndef ASSIMP_TOOLS_H
#define ASSIMP_TOOLS_H

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>

class Renderable;
namespace Assimp{ class Importer; }

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
	 * @details Each Renderable (hopefully) has vertices, normals, uvs and an index buffer, draw mode is GL_TRIANGLES 
	 * @param scene imported with Assimp::Importer
	 * @param vertexTransform transformation that will be apllied to every vertex (and normal). Default: identity
	 */
	std::vector<RenderableInfo > createSimpleRenderablesFromScene( const aiScene* scene, glm::mat4 vertexTransform = glm::mat4(1.0f), bool createTangentsAndBitangents = true); 

	BoundingBox computeBoundingBox(const aiMesh* mesh);

	const aiScene* importAssetFromResourceFolder(std::string filename, Assimp::Importer& importer, int steps = aiProcessPreset_TargetRealtime_MaxQuality);
	
	struct MaterialTextureInfo
	{
		int matIdx;
		int type;
		std::string relativePath;
	};

	enum ScalarType {OPACITY, SHININESS, SHININESS_STRENGTH};
	enum ColorType {AMBIENT, DIFFUSE, SPECULAR, EMISSIVE, REFLECTIVE, TRANSPARENT};
	struct MaterialInfo
	{
		int matIdx;
		std::map< ColorType, glm::vec4> color;
		std::map< ScalarType, float> scalar;
		std::map<aiTextureType, MaterialTextureInfo> texture;
	};

	std::map<aiTextureType, MaterialTextureInfo> getMaterialTexturesInfo(const aiScene* scene, int matIdx);
	MaterialInfo getMaterialInfo(const aiScene* scene, int matIdx);

	std::string decodeColorType(ColorType type);
	std::string decodeScalarType(ScalarType type);
	std::string decodeAiTextureType(aiTextureType type);
	void printMaterialInfo(const MaterialInfo& materialInfo);

	void checkMax(glm::vec3& max, const glm::vec3& point); //!< adapt components of current max by checking them against the components of a point
	void checkMin(glm::vec3& min, const glm::vec3& point); //!< adapt components of current min by checking them against the components of a point

} // namespace AssimpTools
#endif