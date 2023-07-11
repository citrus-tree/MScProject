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
		OPAQUE = 0,
		ALPHA_CLIPPED,
		NORMAL_MAPPED,
		COMPLEX,
	};
	
	enum class SpecialMode
	{
		NONE = 0,
		SCREEN_QUAD_PRESENT,
		SHADOW_MAP
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
		FragmentMode::OPAQUE,
		SpecialMode::NONE,
		{}
	};
}
