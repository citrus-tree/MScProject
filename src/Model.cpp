#include "Model.h"

/* c */
#include <cstring>

/* renderer */
#include "BufferUtilities.hpp"
#include "DescriptorSetLayout.hpp" // <- class DescriptorSetLayout
#include "Environment.hpp" // <- class Environment
#include "Pipeline.hpp" // <- class Pipeline
#include "TextureUtilities.hpp"
#include "Uniforms.hpp"

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"

/* glm */
#include <glm/gtc/type_ptr.hpp>

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
	}

	/* private member functions */

	void Model::loadModel(const char* filepath)
	{
		tinygltf::TinyGLTF file;
		std::string err = "";
		std::string warning = "";

		if (bool res = file.LoadASCIIFromFile(&_model, &err, &warning, filepath); res == false)
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
		/* iterate through the elements of the model (scenes -> meshes) */
		for (size_t s = 0; s < _model.scenes.size(); s++)
		{
			/* per-scene iteration */
			const tinygltf::Scene& cur_scene = _model.scenes[s];

			for (size_t n = 0; n < cur_scene.nodes.size(); n++)
			{
				/* per-node in scene iteration */
				assert(cur_scene.nodes[n] >= 0 && cur_scene.nodes[n] < _model.nodes.size());
				const tinygltf::Node& cur_node = _model.nodes[cur_scene.nodes[n]];

				/* create a queue of the nodes' children */
			}
		}
	}

	/* public member functions */

	void Model::CmdDrawComplex(Environment* environment, Pipeline* pipeline)
	{
	}

	void Model::CmdDrawComplex_DepthOnly(Environment* environment, Pipeline* pipeline)
	{
	}

}