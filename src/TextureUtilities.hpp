#pragma once

/* renderer */
#include "Environment.hpp"

/* labutils */
#include "../labutils/vkimage.hpp"
#include "../labutils/vkobject.hpp"

namespace Renderer
{

	labutils::ImageView CreateImageView(const Environment* environment, VkImage image, VkFormat format);

	void CmdPrimeImageForWrite(Environment* environment, const lut::Image* image, bool isDepth = false);
	void CmdPrimeImageForRead(Environment* environment, const lut::Image* image, bool isDepth = false);
	void CmdTransitionForWrite(Environment* environment, const lut::Image* image, bool isDepth = false);
	void CmdTransitionForRead(Environment* environment, const lut::Image* image, bool isDepth = false);
}
