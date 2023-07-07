#include "BufferUtilities.hpp"

/* c */
#include <cstring>

/* c++ */
#include <limits>

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"

namespace Renderer
{
	void CreateBufferBarrier(VkCommandBuffer commandBuffer, VkBuffer buffer,
		VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
		VkDeviceSize size, VkDeviceSize offset,
		uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex)
	{
		VkBufferMemoryBarrier bbarrier{};
		bbarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bbarrier.srcAccessMask = srcAccessMask;
		bbarrier.dstAccessMask = dstAccessMask;
		bbarrier.buffer = buffer;
		bbarrier.size = size;
		bbarrier.offset = offset;
		bbarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		bbarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;

		vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 1, &bbarrier, 0, nullptr);
	}

	void CreateBuffer(const Environment* environment, labutils::Buffer* oBuffer, uint32_t count, size_t size, void* pData,
		VkBufferUsageFlags vkFlags)
	{
		VkDeviceSize numBytes = count * size;

		/* Buffers on the GPU */
		/* GPU Only! */
		*oBuffer = lut::create_buffer(
			environment->Allocator(),
			numBytes,
			vkFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		/* CPU visible, ready to be mapped! */
		lut::Buffer dataStaging = lut::create_buffer(
			environment->Allocator(),
			numBytes,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);

		/* Map the memory, then copy */
		void* dataPtr = nullptr;
		if (const auto& res = vmaMapMemory(*environment->Allocator(), dataStaging.allocation, &dataPtr); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vmaMapMemory() failed to map the memory buffer. err: %s",
				lut::to_string(res).c_str());
		}
		std::memcpy(dataPtr, pData, numBytes);
		vmaUnmapMemory(*environment->Allocator(), dataStaging.allocation);

		/* The model data is now in the CPU-visible buffers,
		   but we need to copy it to the GPU-exclusive buffers. */

		   /*  Prepare some temporary resources for the staging->exculsive copy */
		lut::Fence copyComplete = lut::create_fence(environment->Window());

		lut::CommandPool copyPool = lut::create_command_pool(environment->Window());
		VkCommandBuffer copyCmd = lut::alloc_command_buffer(environment->Window(), *copyPool);

		/* Queue up the commands */
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (const auto& res = vkBeginCommandBuffer(copyCmd, &beginInfo); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkBeginCommandBuffer() failed to begin the vertex staging->exclusive copy command buffer. err: % s",
				lut::to_string(res).c_str());
		}

		VkBufferCopy dataCopy{};
		dataCopy.size = numBytes;

		vkCmdCopyBuffer(copyCmd, *dataStaging, **oBuffer, 1, &dataCopy);

		CreateBufferBarrier(copyCmd, **oBuffer,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
		);

		if (const auto& res = vkEndCommandBuffer(copyCmd); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkEndCommandBuffer() failed to end the vertex staging->exclusive copy command buffer. err: %s",
				lut::to_string(res).c_str());
		}

		/* Submit 'em! */
		VkSubmitInfo copySubInfo{};
		copySubInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		copySubInfo.commandBufferCount = 1;
		copySubInfo.pCommandBuffers = &copyCmd;

		if (const auto& res = vkQueueSubmit(environment->Window().graphicsQueue, 1, &copySubInfo, *copyComplete); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkQueueSubmit() failed to submit the vertex staging->exclusive copy command buffer. err: %s",
				lut::to_string(res).c_str());
		}

		/* Wait for the fence, so the destruction of local resources is safe. */
		if (const auto& res = vkWaitForFences(environment->Window().device, 1, &*copyComplete, VK_TRUE, std::numeric_limits<uint32_t>::max());
			res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkWaitForFences() failed while attempting to wait for vertex data upload to complete. err: %s",
				lut::to_string(res).c_str());
		}

		/* Done! */
	}

	void FreeUpdateBuffer(const Environment* environment, labutils::Buffer* dstBuffer,
		VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData)
	{
		/* This is very suboptimal, so I only use it during load time to update buffers before the main loop has started */

		lut::Fence updateComplete = lut::create_fence(environment->Window());
		VkCommandBuffer updateCmd = lut::alloc_command_buffer(environment->Window(), *environment->CommandPool());

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (const auto& res = vkBeginCommandBuffer(updateCmd, &beginInfo); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkBeginCommandBuffer() failed. err: %s",
				lut::to_string(res).c_str());
		}

		vkCmdUpdateBuffer(updateCmd, **dstBuffer, dstOffset, dataSize, pData);

		if (const auto& res = vkEndCommandBuffer(updateCmd); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkEndCommandBuffer() failed. err: %s",
				lut::to_string(res).c_str());
		}

		VkSubmitInfo copySubInfo{};
		copySubInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		copySubInfo.commandBufferCount = 1;
		copySubInfo.pCommandBuffers = &updateCmd;

		/* Submit the queued commands */
		if (const auto& res = vkQueueSubmit(environment->Window().graphicsQueue, 1, &copySubInfo, *updateComplete); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkQueueSubmit() failed to submit the vertex staging->exclusive copy command buffer. err: %s",
				lut::to_string(res).c_str());
		}

		/* Wait for the fence. */
		if (const auto& res = vkWaitForFences(environment->Window().device, 1, &*updateComplete, VK_TRUE, std::numeric_limits<uint32_t>::max());
			res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkWaitForFences() failed while attempting to wait for vertex data upload to complete. err: %s",
				lut::to_string(res).c_str());
		}
	}

	void CmdUpdateBuffer(Environment* environment, labutils::Buffer* dstBuffer,
		VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData)
	{
		CreateBufferBarrier(*environment->CurrentCmdBuffer(), **dstBuffer,
			VK_ACCESS_UNIFORM_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		vkCmdUpdateBuffer(*environment->CurrentCmdBuffer(), **dstBuffer, 0, dataSize, pData);
	}
}
