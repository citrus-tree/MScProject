#pragma once

namespace Renderer
{
	/* Pipeline Types and Data */

	enum class AlphaBlend
	{
		DISABLED = 0,
		ENABLED
	};

	enum class FillMode
	{
		FILL = 0,
		WIREFRAME
	};

	enum class FragmentMode
	{
		SIMPLE = 0,
		ALPHA_CLIPPED,
		NORMAL_MAPPED,
		COMPLEX,
	};
	
	enum class SpecialMode
	{
		NONE = 0,
		SCREEN_QUAD_PRESENT,
		POST_PROC_TONE_MAPPING_REINHARD,
		POST_PROC_BLOOM_EXTRACT_HIGHLIGHTS,
		POST_PROC_BLOOM_HORIZONTAL,
		POST_PROC_BLOOM_VERTICAL,
		SHADOW_MAP = 6
	};

	struct PipelineFeatures
	{
		AlphaBlend alphaBlend{};
		FillMode fillMode{};
		FragmentMode fragmentMode{};
		SpecialMode specialMode{};
		std::vector<uint32_t> sideBuffers{};
	};

	/* Default Settings */
	static const PipelineFeatures Pipeline_Default =
	{
		AlphaBlend::DISABLED,
		FillMode::FILL,
		FragmentMode::SIMPLE,
		SpecialMode::NONE,
		{}
	};
}
