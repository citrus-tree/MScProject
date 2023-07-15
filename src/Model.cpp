#include "Model.hpp"

/* c */
#include <cstring>

/* c++ */
#include <list>

/* renderer */
#include "BufferUtilities.hpp"
#include "DescriptorSetLayout.hpp" // <- class DescriptorSetLayout
#include "Environment.hpp" // <- class Environment
#include "Pipeline.hpp" // <- class Pipeline
#include "TextureUtilities.hpp"

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"

/* glm */
#include <glm/gtc/type_ptr.hpp>

/* tinygltf */
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "../ext/tinygltf/include/tiny_gltf.h"

namespace Renderer
{

	/* constructors, etc. */

	Model::Model(const Environment* environment,
		char const* filepath,
		DescriptorSetLayout* descLayout,
		const lut::Sampler* sampler)
	{
		loadModel(filepath);
		createDataVectors(environment, descLayout, sampler);
	}

	Model::~Model()
	{
		if (_model != nullptr)
		{
			delete _model;
		}

		while (_materialData.empty() == false)
		{
			delete _materialData.back().descriptorSet;
			_materialData.pop_back();
		}
	}

	/* private member functions */

	void Model::loadModel(const char* filepath)
	{
		tinygltf::TinyGLTF file;
		std::string err = "";
		std::string warning = "";

		_model = new tinygltf::Model();

		if (bool res = file.LoadBinaryFromFile(_model, &err, &warning, filepath); res == false)
		{
			/* failed to load model */
			printf("RNDR: Failed to load model from file @ %s.",
				filepath);
		}

		if (warning.empty() == false)
		{
			printf("TinyGLTF: LoadASCIIFromFile() returned the following warning: %s",
				warning.c_str());
		}

		if (err.empty() == false)
		{
			printf("TinyGLTF: LoadASCIIFromFile() returned the following error: %s",
				err.c_str());
		}
	}

	void Model::createDataVectors(const Environment* environment, const DescriptorSetLayout* descLayout, const lut::Sampler* sampler)
	{	
		/* iterate through textures */
		_textureData.resize(_model->textures.size());
		for (size_t t = 0; t < _model->textures.size(); t++)
		{
			const tinygltf::Texture& cur_texture = _model->textures[t];
			const tinygltf::Image& cur_teximage = _model->images[cur_texture.source];

			assert(cur_teximage.component <= 4 && cur_teximage.component > 0);
			VkFormat format = srgb_formats[cur_teximage.component];

			/* create texture */
			_textureData[t].texture = lut::image_from_data_texture2d(
				cur_teximage.image, cur_teximage.width, cur_teximage.height, cur_teximage.component,
				environment->Window(), *environment->CommandPool(), environment->Allocator(), format);

			/* create texture image view */
			_textureData[t].textureView = CreateImageView(environment, *_textureData[t].texture, format);
		}

		/* iterate through materials */
		_materialData.resize(_model->materials.size());
		for (size_t m = 0; m < _model->materials.size(); m++)
		{
			const tinygltf::Material& cur_material = _model->materials[m];

			if (cur_material.alphaMode == "MASK")
				_materialData[m].alphaClipped = true;
			else if (cur_material.alphaMode == "BLEND")
				_materialData[m].alphaBlend = true;

			std::vector<Renderer::DescriptorSetFeatures> bindingData{};
			bindingData.resize(3);
			uint32_t bindings = 2;

			/* diffuse texture */
			bindingData[0].binding = 0;
			bindingData[0].s_View = *_textureData[cur_material.pbrMetallicRoughness.baseColorTexture.index].textureView;
			bindingData[0].s_Sampler = **sampler;

			/* metallic roughness texture */
			bindingData[1].binding = 1;
			bindingData[1].s_View = *_textureData[cur_material.pbrMetallicRoughness.metallicRoughnessTexture.index].textureView;
			bindingData[1].s_Sampler = **sampler;

			/* material data */
			Renderer::Uniforms::SimpleMaterial& material = _materialData[m].data;
			material.data.inner_data.abledo = 
				glm::vec4(
					cur_material.pbrMetallicRoughness.baseColorFactor[0],
					cur_material.pbrMetallicRoughness.baseColorFactor[1],
					cur_material.pbrMetallicRoughness.baseColorFactor[2],
					cur_material.pbrMetallicRoughness.baseColorFactor[3]);
			material.data.inner_data.emissive =
				glm::vec3(
					cur_material.emissiveFactor[0],
					cur_material.emissiveFactor[1],
					cur_material.emissiveFactor[2]);
			material.data.inner_data.roughness =
				static_cast<float>(cur_material.pbrMetallicRoughness.roughnessFactor);
			material.data.inner_data.metallic =
				static_cast<float>(cur_material.pbrMetallicRoughness.metallicFactor);
			CreateBuffer(environment, &_materialData[m].dataBuffer, 1, sizeof(float) * 16, &_materialData[m].data, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

			bindingData[2].binding = 2;
			bindingData[2].u_Buffer = *_materialData[m].dataBuffer;

			_materialData[m].descriptorSet = new Renderer::DescriptorSet(environment, descLayout,
				static_cast<uint32_t>(bindingData.size()), bindingData.data());

			/* should be all for now */
		}

		/* iterate through meshes */
		_meshes.resize(0);
		int offset = 0;
		for (size_t m = 0; m < _model->materials.size(); m++)
		{
			const tinygltf::Mesh& mesh = _model->meshes[m];

			_meshes.resize(_meshes.size() + mesh.primitives.size());

			for (size_t p = 0; p < mesh.primitives.size(); p++)
			{
				const tinygltf::Primitive& primitive = mesh.primitives[p];

				/* enqueue the mesh primitive in the appropriate vector of primitives */
				if (_materialData[primitive.material].alphaBlend == false)
					_opaqueMeshes.push_back(offset);
				else
					_transparentMeshes.push_back(offset);

				/* upload position data */
				uint32_t acc_index = primitive.attributes.at("POSITION");

				tinygltf::Accessor accessor = _model->accessors[acc_index];
				tinygltf::BufferView bufferView = _model->bufferViews[accessor.bufferView];
				tinygltf::Buffer buffer = _model->buffers[bufferView.buffer];

				CreateBuffer(
					environment, &_meshes[offset].positions, static_cast<uint32_t>(accessor.count),
					static_cast<uint32_t>(bufferView.byteLength / accessor.count),
					buffer.data.data() + bufferView.byteOffset);

				/* upload normal data */
				acc_index = primitive.attributes.at("NORMAL");

				accessor = _model->accessors[acc_index];
				bufferView = _model->bufferViews[accessor.bufferView];
				buffer = _model->buffers[bufferView.buffer];

				CreateBuffer(
					environment, &_meshes[offset].normals, static_cast<uint32_t>(accessor.count),
					static_cast<uint32_t>(bufferView.byteLength / accessor.count),
					buffer.data.data() + bufferView.byteOffset);

				/* upload uv data */
				acc_index = primitive.attributes.at("TEXCOORD_0");

				accessor = _model->accessors[acc_index];
				bufferView = _model->bufferViews[accessor.bufferView];
				buffer = _model->buffers[bufferView.buffer];

				CreateBuffer(
					environment, &_meshes[offset].uvs, static_cast<uint32_t>(accessor.count),
					static_cast<uint32_t>(bufferView.byteLength / accessor.count),
					buffer.data.data() + bufferView.byteOffset);

				/* upload indices data */
				acc_index = primitive.indices;

				accessor = _model->accessors[acc_index];
				bufferView = _model->bufferViews[accessor.bufferView];
				buffer = _model->buffers[bufferView.buffer];

				CreateBuffer(
					environment, &_meshes[offset].indices, static_cast<uint32_t>(accessor.count),
					static_cast<uint32_t>(bufferView.byteLength / accessor.count),
					buffer.data.data() + bufferView.byteOffset,
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

				_meshes[offset].indicesSize = static_cast<uint32_t>(accessor.count);

				/* assign material */
				_meshes[offset].materialIndex = primitive.material;

				/* increment the offset */
				offset++;
			}
		}

		///* create queues of nodes */
		//std::list<int> todo_nodes = {};
		//std::list<int> comp_nodes = {};

		///* iterate through the elements of the model (scenes -> meshes) */
		//for (size_t s = 0; s < _model->scenes.size(); s++)
		//{
		//	/* per-scene iteration */
		//	const tinygltf::Scene& cur_scene = _model->scenes[s];

		//	for (size_t n = 0; n < cur_scene.nodes.size(); n++)
		//	{
		//		/* per-node in scene iteration */
		//		assert(cur_scene.nodes[n] >= 0 && cur_scene.nodes[n] < _model->nodes.size());
		//		const tinygltf::Node& cur_node = _model->nodes[cur_scene.nodes[n]];

		//		/* create a queue of the nodes' children */
		//	}
		//}
	}

	/* public member functions */

	void Model::CmdDrawOpaque(Environment* environment, Pipeline* pipeline, bool materialOverriden)
	{
		CmdDrawOpaque(environment, pipeline, 0, _opaqueMeshes.size(), materialOverriden);
	}

	void Model::CmdDrawOpaque(Environment* environment, Pipeline* pipeline, size_t start, size_t end, bool materialOverriden)
	{
		for (size_t i = start; i < end; i++)
		{
			const MeshData& cur_mesh = _meshes[_opaqueMeshes[i]];

			VkBuffer buffers[3] = { *cur_mesh.positions, *cur_mesh.uvs, *cur_mesh.normals };
			VkDeviceSize offsets[3]{ 0, 0, 0 };

			if (materialOverriden == false)
				_materialData[_meshes[_opaqueMeshes[i]].materialIndex].descriptorSet->CmdBind(environment, pipeline, 1);

			vkCmdBindVertexBuffers(*environment->CurrentCmdBuffer(), 0, 3, buffers, offsets);
			vkCmdBindIndexBuffer(*environment->CurrentCmdBuffer(), *cur_mesh.indices, 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(*environment->CurrentCmdBuffer(), cur_mesh.indicesSize, 1, 0, 0, 0);
		}
	}

	void Model::CmdDrawOpaque_DepthOnly(Environment* environment, Pipeline* pipeline)
	{
		CmdDrawOpaque_DepthOnly(environment, pipeline, 0, _opaqueMeshes.size());
	}

	void Model::CmdDrawOpaque_DepthOnly(Environment* environment, Pipeline* pipeline, size_t start, size_t end)
	{
		for (size_t i = start; i < end; i++)
		{
			const MeshData& cur_mesh = _meshes[_opaqueMeshes[i]];

			VkBuffer buffers[2] = { *cur_mesh.positions, *cur_mesh.uvs };
			VkDeviceSize offsets[2]{ 0, 0 };

			vkCmdBindVertexBuffers(*environment->CurrentCmdBuffer(), 0, 2, buffers, offsets);
			vkCmdBindIndexBuffer(*environment->CurrentCmdBuffer(), *cur_mesh.indices, 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(*environment->CurrentCmdBuffer(), cur_mesh.indicesSize, 1, 0, 0, 0);
		}
	}

}