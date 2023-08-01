#pragma once

#include "SharedFeatures.hpp"

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
		SIMPLE = 0
	};
	
	enum class SpecialMode
	{
		NONE = 0,
		SCREEN_QUAD_PRESENT,
		SHADOW_MAP,
		TS_GEOMETRY,
		TS_COLOURED_SHADOW_MAP,
		SSM_STOCHASTIC_SHADOW_MAP,
		SSM_DEFAULT_BIG_PCF,
		CSSM_COLORED_STOCHASTIC_SHADOW_MAP,
		CSSM_COLORED_STOCHASTIC_SHADOW_MAP_2,
		CSSM_DEFAULT,
		DPTS_GEOMETRY,
		DPTS_SHADOWMAP
	};

	enum class DepthWrite
	{
		ENABLED = 0,
		DISABLED
	};

	enum class DepthOp
	{
		LEQUAL = 0,
		GEQUAL,
		LESS,
		GREATER
	};

	enum class ColorWrite
	{
		ENABLED = 0,
		DISABLED
	};

	enum class BlendMode
	{
		ADD_SRC_ONEMINUSSRC = 0,
		MIN_ONE_ONE
	};

	struct PipelineFeatures
	{
		AlphaBlend alphaBlend{};
		FillMode fillMode{};
		FragmentMode fragmentMode{};
		SpecialMode specialMode{};
		DepthWrite depthWrite{};
		DepthTest depthTest{};
		DepthOp depthOp{};
		ColorWrite colorWrite{};
		BlendMode blendMode{};
		std::vector<uint32_t> sideBuffers{};
	};

	/* Default Settings */
	static const PipelineFeatures Pipeline_Default =
	{
		AlphaBlend::DISABLED,
		FillMode::FILL,
		FragmentMode::SIMPLE,
		SpecialMode::NONE,
		DepthWrite::ENABLED,
		DepthTest::ENABLED,
		DepthOp::LEQUAL,
		ColorWrite::ENABLED,
		BlendMode::ADD_SRC_ONEMINUSSRC,
		{}
	};
}
