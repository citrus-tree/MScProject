#pragma once

/* c++ */
#include <vector>
#include <string>

/* labutils */
#include "../labutils/vkbuffer.hpp"
#include "../labutils/vkimage.hpp"

/* renderer */
#include "DescriptorSets.hpp"
#include "Uniforms.hpp"

namespace Renderer
{
	class DescriptorSetLayout;
	class Environment;
	class Pipeline;
}

namespace tinygltf
{
	class Model;
	struct Accessor;
	struct BufferView;
	struct Buffer;
}

namespace Renderer
{
	namespace lut = labutils;

	class Model
	{
		public:
			/* constructors, etc. */

			Model() = delete;
			Model(const Environment* environment,
				char const* filepath,
				DescriptorSetLayout* descLayout,
				const lut::Sampler* sampler);
			~Model();

			Model(const Model&) = delete;
			Model& operator=(const Model&) = delete;

		private:
			/* private member variables */

			std::vector<VkFormat> srgb_formats =
			{
				VK_FORMAT_R8G8B8A8_SRGB,	// 0
				VK_FORMAT_R8_SRGB,			// 1
				VK_FORMAT_R8G8_SRGB,		// 2
				VK_FORMAT_R8G8B8_SRGB,		// 3
				VK_FORMAT_R8G8B8A8_SRGB,	// 4
			};

			std::vector<VkFormat> unorm_formats =
			{
				VK_FORMAT_R8G8B8A8_UNORM,	// 0
				VK_FORMAT_R8_UNORM,			// 1
				VK_FORMAT_R8G8_UNORM,		// 2
				VK_FORMAT_R8G8B8_UNORM,		// 3
				VK_FORMAT_R8G8B8A8_UNORM,	// 4
			};

			struct TextureData
			{
				bool isNormalMap = false;
				lut::Image texture{};
				lut::ImageView textureView{};
			};

			struct MaterialData
			{
				bool normalMapped = false;
				bool alphaClipped = false;
				bool alphaBlend = false;

				Renderer::Uniforms::SimpleMaterial data;
				lut::Buffer dataBuffer{};

				DescriptorSet* descriptorSet{};
			};

			struct MeshData
			{
				lut::Buffer positions{}; // vec3
				lut::Buffer uvs{}; // vec2
				lut::Buffer normals{}; // vec3

				lut::Buffer indices{}; // uint32_t

				uint32_t indicesSize = 0;
				DescriptorSet* descriptorSet{};

				glm::vec3 centerPt = glm::vec3(0);

				int materialIndex = -1;
			};

			std::vector<TextureData> _textureData{};
			std::vector<MaterialData> _materialData{};
			std::vector<MeshData> _meshes{};

			std::vector<int> _opaqueMeshes{};
			std::vector<int> _transparentMeshes{};
			std::vector<int> _transparentMeshesSortedClosestToLight{};
			std::vector<int> _transparentMeshesSortedClosestToLightInverse{};
			std::vector<int> _transparentMeshesSortedFarthestFromCamera{};
			tinygltf::Model* _model = nullptr;

			/* private member functions */

			void loadModel(const char* filepath);

			void createDataVectors(const Environment* environment,
				const DescriptorSetLayout* descLayout,
				const lut::Sampler* sampler);

			glm::vec3 calculateAveragePoint(const tinygltf::Accessor* accessor,
				const tinygltf::BufferView* bufferView,
				tinygltf::Buffer* buffer);

			public:

			/* public member functions */

			void CmdDrawOpaque(Environment* environment, Pipeline* pipeline, bool materialOverriden = false);
			void CmdDrawOpaque(Environment* environment, Pipeline* pipeline, size_t start, size_t end, bool materialOverriden = false);
			void CmdDrawOpaque_DepthOnly(Environment* environment, Pipeline* pipeline);
			void CmdDrawOpaque_DepthOnly(Environment* environment, Pipeline* pipeline, size_t start, size_t end);

			void CmdDrawTransparent(Environment* environment, Pipeline* pipeline, bool materialOverriden = false);
			void CmdDrawTransparent(Environment* environment, Pipeline* pipeline, size_t start, size_t end, bool materialOverriden = false);
			void CmdDrawTransparent_DepthOnly(Environment* environment, Pipeline* pipeline);
			void CmdDrawTransparent_DepthOnly(Environment* environment, Pipeline* pipeline, size_t start, size_t end);

			void SortTransparentGeometry(glm::vec3 lightPosition, glm::vec3 cameraPosition, bool sortLight = true, bool sortCamera = true);

			void CmdDrawTransparentLightFrontToBack(Environment* environment, Pipeline* pipeline, bool materialOverriden = false);
			void CmdDrawTransparentLightFrontToBack(Environment* environment, Pipeline* pipeline, size_t start, size_t end, bool materialOverriden = false);
			void CmdDrawTransparentLightFrontToBack_DepthOnly(Environment* environment, Pipeline* pipeline, bool materialOverriden = false);
			void CmdDrawTransparentLightFrontToBack_DepthOnly(Environment* environment, Pipeline* pipeline, size_t start, size_t end, bool materialOverriden = false);
			void CmdDrawTransparentCameraBackToFront(Environment* environment, Pipeline* pipeline, bool materialOverriden = false);
			void CmdDrawTransparentCameraBackToFront(Environment* environment, Pipeline* pipeline, size_t start, size_t end, bool materialOverriden = false);
	
			inline uint32_t MeshCount() { return static_cast<uint32_t>(_meshes.size()); }
			inline uint32_t OpaqueMeshCount() { return static_cast<uint32_t>(_opaqueMeshes.size()); }
			inline uint32_t TransparentMeshCount() { return static_cast<uint32_t>(_transparentMeshes.size()); }
			inline const std::vector<int>& TransparentMeshesSortedClosestToLight() { return _transparentMeshesSortedClosestToLight; }
			inline const std::vector<int>& TransparentMeshesSortedFarthestFromCamera() { return _transparentMeshesSortedFarthestFromCamera; }
			inline const uint32_t ReverseLookupTransparentMeshSortedClosestToLight(int i) { return _transparentMeshesSortedClosestToLightInverse[i]; }
	};
}
