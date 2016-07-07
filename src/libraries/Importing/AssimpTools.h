#ifndef ASSIMP_TOOLS_H
#define ASSIMP_TOOLS_H

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <string>

class Renderable;
namespace Assimp{ class Importer; }

namespace AssimpTools {
	struct EnumClassHash
	{
	    template <typename T>
	    std::size_t operator()(T t) const
	    {
	        return static_cast<std::size_t>(t);
	    }
	};

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
	std::vector<RenderableInfo > createSimpleRenderablesFromScene( const aiScene* scene, const glm::mat4& vertexTransform = glm::mat4(1.0f), bool createTangentsAndBitangents = true); 

	struct VertexData
	{
		std::vector<unsigned int> indices;
		std::vector<float> positions;
		std::vector<float> uvs;
		std::vector<float> normals;
		std::vector<float> tangents;
	};
	std::vector<VertexData> createVertexDataInstancesFromScene( const aiScene* scene, const glm::mat4& vertexTransform = glm::mat4(1.0f), bool createTangentsAndBitangents = true);
	std::vector<Renderable* > createSimpleRenderablesFromVertexDataInstances(std::vector<VertexData>& vertexDataInstances);

	BoundingBox computeBoundingBox(const aiMesh* mesh); //!< computes the bounding box of the given mesh
	BoundingBox computeBoundingBox(VertexData& mesh); //!< computes the bounding box of the given mesh

	/**
	* @brief imports an asset from the resource folder
	* @details just to save some lines of code: uses Importer.ReadFile() and produces some output if an error occures
	* 
	* @param filename given relative to RESOURCES_PATH, e.g. "cube.obj"
	* @param importer instance to bind this scene to
	* @param steps flags form the aiPostProcessSteps namespace
	* @return const pointer to the aiScene
	*/
	const aiScene* importAssetFromResourceFolder(std::string filename, Assimp::Importer& importer, int steps = aiProcessPreset_TargetRealtime_MaxQuality);
	
	/** @brief Struct that saves info about a texture used by a material. */
	struct MaterialTextureInfo
	{
		int matIdx; //!< material idx this texture belongs to
		int type;   //!< type of this texture, one of the aiTextureType name space, e.g. aiTextureType_DIFFUSE or aiTextureType_NORMALS
		std::string relativePath; //!< texture file path relative to the corresponding asset's file location
	};

	enum ScalarType {OPACITY, SHININESS, SHININESS_STRENGTH, REFLECTIVITY}; //!<enum identifying a scalar property of the material
	enum ColorType {AMBIENT, DIFFUSE, SPECULAR, EMISSIVE, REFLECTIVE, TRANSPARENT}; //!< enum identifying a color property of the material
	
	/** @brief struct that saves all defined properties of a material imported with an asset. */
	struct MaterialInfo
	{
		int matIdx; //!< material index in the corresponding aiScene object
		std::unordered_map< ColorType, glm::vec4, EnumClassHash> color; //!< all color properties that have been defined for this material
		std::unordered_map< ScalarType, float, EnumClassHash> scalar; //!< all scalar properties that have been defined for this material
		std::unordered_map<aiTextureType, MaterialTextureInfo, EnumClassHash> texture; //!< all texture information that have been defined for this material
	};

	std::unordered_map<aiTextureType, MaterialTextureInfo, EnumClassHash> getMaterialTexturesInfo(const aiScene* scene, int matIdx); //!< retrieve properties of every texture used by this material
	MaterialInfo getMaterialInfo(const aiScene* scene, int matIdx); //!< retrieve properties of a certain material of the corresponding aiScene

	std::string decodeColorType(ColorType type); //!< get a readable string for the provided color type
	std::string decodeScalarType(ScalarType type); //!< get a readable string for the provided scalar type
	std::string decodeAiTextureType(aiTextureType type); //!< get a readable string for the provided texture type
	void printMaterialInfo(const MaterialInfo& materialInfo); //!< print everything contained in a material's information struct

	void checkMax(glm::vec3& max, const glm::vec3& point); //!< adapt components of current max by checking them against the components of a point
	void checkMin(glm::vec3& min, const glm::vec3& point); //!< adapt components of current min by checking them against the components of a point

} // namespace AssimpTools
#endif