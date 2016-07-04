#include <Rendering/VertexArrayObjects.h>
#include <Importing/AssimpTools.h>
#include <Rendering/RenderPass.h>
#include <Core/Camera.h>

namespace Culling{

struct CullingInfo
{
	AssimpTools::BoundingBox boundingBox; // local space
	float boundingRadius;   // radius from center
	glm::mat4 modelMatrix; // pose of center
};

std::pair<Renderable*, CullingInfo> getCullingInfo(const AssimpTools::RenderableInfo& renderable, const glm::mat4& modelMatrix);
std::vector<std::pair<Renderable*, CullingInfo> > getCullingInfo(const std::vector<AssimpTools::RenderableInfo>& renderables, const glm::mat4& modelMatrix);
std::vector<std::pair<Renderable*, CullingInfo> > getCullingInfo(const std::vector<AssimpTools::RenderableInfo>& renderables, const std::vector<glm::mat4> modelMatrices );

class CullingHelper{
public:
	CullingHelper(Camera* camera = nullptr);
	~CullingHelper();

	std::vector<Renderable* > cullAgainstFrustum( const std::vector<std::pair<Renderable*, CullingInfo> >& renderables);
	std::vector<glm::mat4 >   cullAgainstFrustum( const std::vector< CullingInfo>& instances);

	std::vector<Renderable* > cullOccluded( const std::vector<std::pair<Renderable*, CullingInfo> >& renderables);
	std::vector<glm::mat4 > cullOccluded( const std::vector< CullingInfo>& instances);

	void setCamera(Camera* camera);

	enum Visibility {OUTSIDE = 0, INTERSECTING, INSIDE};
	int pointInFrustum(glm::vec3 point);
	int boxInFrustum(glm::vec3 min, glm::vec3 max);
	int sphereInFrustum(glm::vec3 point, float radius);

	void updateFrustumShape();
	void updateFrustumPosition();
	void updateFrustum(); // both of above

protected:
	Camera* m_camera;

	struct Frustum
	{

	} m_frustum;

};
} // namespace Culling