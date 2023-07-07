#pragma once

/* renderer */
#include "Environment.hpp"

/* labutils */
#include "../labutils/vkimage.hpp"
#include "../labutils/vkobject.hpp"

/* volk */
#include <volk/volk.h>

namespace Renderer
{
	inline void CmdDrawFullscreenQuad(Environment* environment)
	{
		vkCmdDraw(*environment->CurrentCmdBuffer(), 3, 1, 0, 0);
	}
}
