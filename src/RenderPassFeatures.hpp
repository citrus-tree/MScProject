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

	struct RenderPassFeatures
	{
		ColourPass colourPass{};
		DepthTest depthTest{};
		RenderTarget renderTarget{};
	};

	/* Default Settings */
	static const RenderPassFeatures RenderPass_Default  =
	{
		ColourPass::ENABLED,
		DepthTest::ENABLED,
		RenderTarget::PRESENT
	};
}
