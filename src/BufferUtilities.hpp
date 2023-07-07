#pragma once

#pragma once

/* renderer */ 
#include "Environment.hpp"

/* labutils */
#include "../labutils/vkbuffer.hpp"

namespace Renderer
{
	/* Creates and submits a memory barrier to the given queue */
	void CreateBufferBarrier(VkCommandBuffer commandBuffer, VkBuffer buffer,
		VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
		VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0,
		uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

	/* Creates a buffer on the GPU */
	void CreateBuffer(const Environment* environment, labutils::Buffer* oBuffer, uint32_t count, size_t element_size, void* pData,
		VkBufferUsageFlags vkFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	/* For use when there is no actively recording command buffer */
	void FreeUpdateBuffer(const Environment* environment, labutils::Buffer* dstBuffer,
		VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);

	/* For use when a command buffer is currently recording commands */
	void CmdUpdateBuffer(Environment* environment, labutils::Buffer* dstBuffer,
		VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
}