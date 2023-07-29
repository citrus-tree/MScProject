#pragma once

#include "SharedFeatures.hpp"

namespace Renderer
{
	/* Render Pass Types and Data */

	enum class ColourPass
	{
		DISABLED = 0,
		ENABLED
	};

	enum class RenderTarget
	{
		PRESENT = 0,
		TEXTURE_GEOMETRY,
		TEXTURE_POST_PROC,
		TEXTURE_SHADOWMAP,
		TEXTURE_COLORDEPTH
	};

	enum class SpecialColour
	{
		NONE = 0,
		CSSM_SHADOWMAP
	};

	enum class ClearDepth
	{
		ENABLED = 0,
		DISABLED
	};

	enum class ClearColour
	{
		ENABLED = 0,
		DISABLED
	};

	struct RenderPassFeatures
	{
		ColourPass colourPass{};
		DepthTest depthTest{};
		RenderTarget renderTarget{};
		SpecialColour specialColour{};
		ClearDepth clearDepth{};
		ClearColour clearColour{};
	};

	/* Default Settings */
	static const RenderPassFeatures RenderPass_Default  =
	{
		ColourPass::ENABLED,
		DepthTest::ENABLED,
		RenderTarget::PRESENT,
		SpecialColour::NONE,
		ClearDepth::ENABLED,
		ClearColour::ENABLED
	};
}
