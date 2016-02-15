#include "CullingTools.h"

Culling::CullingHelper::CullingHelper(Camera* camera)
	: m_camera(camera)
{
}

Culling::CullingHelper::~CullingHelper()
{
}

std::pair<Renderable*, Culling::CullingInfo> Culling::getCullingInfo(const AssimpTools::RenderableInfo& renderable, glm::mat4 modelMatrix)
{
	CullingInfo cullingInfo;
	
	cullingInfo.boundingBox = renderable.boundingBox;
	
	glm::vec3 center = cullingInfo.boundingBox.min + 0.5f * (cullingInfo.boundingBox.max - cullingInfo.boundingBox.min);
	cullingInfo.boundingRadius = glm::length(0.5f * (cullingInfo.boundingBox.max - cullingInfo.boundingBox.min));
	
	cullingInfo.modelMatrix = modelMatrix;

	return std::pair<Renderable*, Culling::CullingInfo>(renderable.renderable, cullingInfo);
}

std::vector<std::pair<Renderable*, Culling::CullingInfo> > Culling::getCullingInfo(const std::vector<AssimpTools::RenderableInfo>& renderables, glm::mat4 modelMatrix)
{
	std::vector<std::pair<Renderable*, CullingInfo>> result;

	for (auto r : renderables)
	{
		result.push_back( getCullingInfo( r, modelMatrix ) );	
	}

	return result;
}

std::vector<std::pair<Renderable*, Culling::CullingInfo> > Culling::getCullingInfo(const std::vector<AssimpTools::RenderableInfo>& renderables, const std::vector<glm::mat4> modelMatrices )
{
	std::vector<std::pair<Renderable*, CullingInfo>> result;
	
	if ( modelMatrices.empty() )
	{
		DEBUGLOG->log("WARNING: no matrices assigned, will use identity matrix instead");
		return getCullingInfo(renderables, glm::mat4(1.0f));
	}

	if ( modelMatrices.size() < renderables.size() )
	{
		DEBUGLOG->log("WARNING: fewer matrices than renderables, matrices will be reused using modulo");
	}

	unsigned int i = 0;
	for (auto r : renderables)
	{
		result.push_back( getCullingInfo( r, modelMatrices.at(i) ) );	
		
		i = (i++) % modelMatrices.size();
	}

	return result;
}

std::vector<Renderable* > Culling::CullingHelper::cullAgainstFrustum( const std::vector<std::pair<Renderable*, Culling::CullingInfo> >& renderables )
{
	std::vector<Renderable* > result;	
	if ( m_camera == nullptr)
	{
		DEBUGLOG->log("WARNING: no camera info. No renderables will be culled.");
		for ( auto e : renderables ) {result.push_back(e.first);}
		return result;
	}

	for ( auto e : renderables )
	{
		int visibility = OUTSIDE;
		
		glm::vec3 sphereCenter = glm::vec3(e.second.modelMatrix[3]) + (e.second.boundingBox.min + 0.5f * (e.second.boundingBox.max - e.second.boundingBox.min)); 
		int sphereVisibility = sphereInFrustum ( sphereCenter, e.second.boundingRadius );
		if ( sphereVisibility != OUTSIDE )
		{
			visibility = sphereVisibility;
		}
		
		if ( sphereVisibility == INTERSECTING )
		{

		}

		if ( visibility != OUTSIDE)
		{
			result.push_back(e.first);
		}

	}


	return result;
}

std::vector<glm::mat4 > Culling::CullingHelper::cullAgainstFrustum( const std::vector< Culling::CullingInfo>& instances )
{
	std::vector<glm::mat4 > result;
	if ( m_camera == nullptr)
	{
		DEBUGLOG->log("WARNING: no camera info. No instances will be culled.");
		for ( auto e : instances ) {result.push_back(e.modelMatrix);}
		return result;
	}
	return result;
}

std::vector<Renderable* > Culling::CullingHelper::cullOccluded( const std::vector<std::pair<Renderable*, Culling::CullingInfo> >& renderables )
{
	std::vector<Renderable* > result;
	if ( m_camera == nullptr)
	{
		DEBUGLOG->log("WARNING: no camera info. No renderables will be culled.");
		for ( auto e : renderables ) {result.push_back(e.first);}
		return result;
	}
	return result;
}

std::vector<glm::mat4 > Culling::CullingHelper::cullOccluded( const std::vector< Culling::CullingInfo>& instances )
{
	std::vector<glm::mat4 > result;	
	if ( m_camera == nullptr)
	{
		DEBUGLOG->log("WARNING: no camera info. No instances will be culled.");
		for ( auto e : instances ) {result.push_back(e.modelMatrix);}
		return result;
	}
	return result;
}

void Culling::CullingHelper::setCamera( Camera* camera )
{
	m_camera = camera;
}

int Culling::CullingHelper::sphereInFrustum(glm::vec3 point, float radius)
{
	if ( m_camera == nullptr)
	{
		DEBUGLOG->log("WARNING: no camera info. Returning INSIDE");
		return INSIDE;
	}

	glm::vec4 viewPoint = *(m_camera->getViewMatrixPointer()) * glm::vec4(point, 1.0f);

	if (viewPoint.z - radius > 0.0f) // behind camera
	{
		if ( viewPoint.z - 2.0f * radius > 0.0f ) // full diameter distance to z == 0.0 
		{
			return OUTSIDE;
		}
		else
		{
			return INTERSECTING;
		}
	}

	// TODO stuff

	return INSIDE;
}
