#pragma once

namespace Renderer
{
	/* Render Pass Types and Data */

	enum class ColourPass
	{
		DISABLED = 0,
		ENABLED
	};

	enum class DepthTest
	{
		DISABLED = 0, // currently setting this to disabled works, but causes warnings
		ENABLED
	};

	enum class RenderTarget
	{
		PRESENT = 0,
		TEXTURE_GEOMETRY,
		TEXTURE_POST_PROC,
		TEXTURE_SHADOWMAP
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
