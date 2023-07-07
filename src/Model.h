#pragma once

/* c++ */
#include <vector>
#include <string>

/* labutils */
#include "../labutils/vkbuffer.hpp"
#include "../labutils/vkimage.hpp"

/* renderer */
#include "DescriptorSets.hpp"

/* tinygltf */
#include "../ext/tinygltf/include/tiny_gltf.h"

namespace Renderer
{
	class DescriptorSetLayout;
	class Environment;
	class Pipeline;
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

				DescriptorSet* descriptorSet{};
			};

			struct MeshData
			{
				lut::Buffer positions{}; // vec3
				lut::Buffer uvs{}; // vec2
				lut::Buffer normals{}; // vec3

				lut::Buffer indices{}; // uint32_t

				uint32_t meshDataIndex = 0;
				DescriptorSet* descriptorSet{};
			};

			std::vector<TextureData> _textureData{};
			std::vector<MaterialData> _materialData{};
			std::vector<MeshData> _meshes{};
			tinygltf::Model _model{};

			/* private member functions */

			void loadModel(const char* filepath);

			void createDataVectors(const Environment* environment,
				const DescriptorSetLayout* descLayout,
				const lut::Sampler* sampler);

			public:
			/* public member functions */

			void CmdDrawComplex(Environment* environment, Pipeline* pipeline);
			void CmdDrawComplex_DepthOnly(Environment* environment, Pipeline* pipeline);
	};
}
