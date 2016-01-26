#include <Rendering/RenderPass.h>
class Quad;

namespace PostProcessing
{
	/*Box Blur using quadric interpolation*/
	class BoxBlur
	{
	public:
		BoxBlur(int width, int height, Quad* quad = nullptr);
		~BoxBlur();

		std::vector<GLuint> m_mipmapFBOHandles;
		GLuint m_mipmapTextureHandle;

		ShaderProgram m_pushShaderProgram;
		
		void pull(); // generate mipmaps from current content of level 0
		void push(int numLevels, int beginLevel = 0); // blur levels (beginLevel + numLevels) down to beginLevel

	private:
		Quad* m_quad;
		bool ownQuad;
	};
} // namespace PostProcessing
