#include "vkimage.hpp"

#include <limits>
#include <vector>
#include <utility>
#include <algorithm>

#include <cstdio>
#include <cassert>
#include <cstring> // for std::memcpy()

#include <stb_image.h>

#include "error.hpp"
#include "vkutil.hpp"
#include "vkbuffer.hpp"
#include "to_string.hpp"



namespace
{
	// Unfortunately, std::countl_zero() isn't available in C++17; it was added
	// in C++20. This provides a fallback implementation. Unlike C++20, this
	// returns a std::uint32_t and not a signed int.
	//
	// See https://graphics.stanford.edu/~seander/bithacks.html for this and
	// other methods like it.
	//
	// Note: that this is unlikely to be the most efficient implementation on
	// most processors. Many instruction sets have dedicated instructions for
	// this operation. E.g., lzcnt (x86 ABM/BMI), bsr (x86).
	inline
		std::uint32_t countl_zero_(std::uint32_t aX)
	{
		if (!aX) return 32;

		uint32_t res = 0;

		if (!(aX & 0xffff0000)) (res += 16), (aX <<= 16);
		if (!(aX & 0xff000000)) (res += 8), (aX <<= 8);
		if (!(aX & 0xf0000000)) (res += 4), (aX <<= 4);
		if (!(aX & 0xc0000000)) (res += 2), (aX <<= 2);
		if (!(aX & 0x80000000)) (res += 1);

		return res;
	}
}

namespace labutils
{
	Image::Image() noexcept = default;

	Image::~Image()
	{
		if (VK_NULL_HANDLE != image)
		{
			assert(VK_NULL_HANDLE != mAllocator);
			assert(VK_NULL_HANDLE != allocation);
			vmaDestroyImage(mAllocator, image, allocation);
		}
	}

	Image::Image(VmaAllocator aAllocator, VkImage aImage, VmaAllocation aAllocation) noexcept
		: image(aImage)
		, allocation(aAllocation)
		, mAllocator(aAllocator)
	{}

	Image::Image(Image&& aOther) noexcept
		: image(std::exchange(aOther.image, VK_NULL_HANDLE))
		, allocation(std::exchange(aOther.allocation, VK_NULL_HANDLE))
		, mAllocator(std::exchange(aOther.mAllocator, VK_NULL_HANDLE))
	{}
	Image& Image::operator=(Image&& aOther) noexcept
	{
		std::swap(image, aOther.image);
		std::swap(allocation, aOther.allocation);
		std::swap(mAllocator, aOther.mAllocator);
		return *this;
	}
	const VkImage& Image::operator*() const
	{
		return image;
	}
}

namespace labutils
{
	Image load_image_texture2d(char const* aPath, VulkanContext const& aContext, VkCommandPool aCmdPool, Allocator const& aAllocator, VkFormat format)
	{
		stbi_set_flip_vertically_on_load(1);

		int iwidth, iheight, ichannels;
		stbi_uc* data = stbi_load(aPath, &iwidth, &iheight, &ichannels, 4);

		if (data == nullptr)
		{
			throw Error("STBI: stbi_load() failed to load image at [%s]. err: %s",
				aPath, stbi_failure_reason());
		}

		const uint32_t uwidth = static_cast<uint32_t>(iwidth);
		const uint32_t uheight = static_cast<uint32_t>(iheight);
		const uint32_t ubytes = uwidth * uheight * 4U;

		Buffer stagingBuff = create_buffer(aAllocator, ubytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		void* stagingPtr = nullptr;
		if (const auto& res = vmaMapMemory(*aAllocator, stagingBuff.allocation, &stagingPtr); res != VK_SUCCESS)
		{
			throw Error("VK: vmaMapMemory() failed to allocate the staging buffer for texture upload. err: %s",
				to_string(res).c_str());
		}

		memcpy(stagingPtr, data, ubytes);
		vmaUnmapMemory(*aAllocator, stagingBuff.allocation);

		stbi_image_free(data);

		Image ret = create_image_texture2d(aAllocator, uwidth, uheight, format,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		VkCommandBuffer cmdBuff = alloc_command_buffer(aContext, aCmdPool);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (const auto& res = vkBeginCommandBuffer(cmdBuff, &beginInfo); res != VK_SUCCESS)
		{
			throw Error("VK: vkBeginCommandBuffer() failed. err: %s",
				to_string(res).c_str());
		}

		auto const mipLevels = compute_mip_level_count(uwidth, uheight);

		/* BARRIER: UNDEFINED -> TRANSFER_DST */
		image_barrier(cmdBuff, ret.image,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, mipLevels,
				0, 1
			});

		VkBufferImageCopy icopy{};
		icopy.bufferOffset = 0;
		icopy.bufferRowLength = 0;
		icopy.bufferImageHeight = 0;
		icopy.imageSubresource = VkImageSubresourceLayers
		{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0,
			0, 1
		};
		icopy.imageOffset = VkOffset3D{ 0, 0, 0 };
		icopy.imageExtent = VkExtent3D{ uwidth, uheight, 1 };

		vkCmdCopyBufferToImage(cmdBuff, *stagingBuff, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &icopy);

		/* BARRIER: TRANSFER_DST -> TRANSFER_SRC */
		image_barrier(cmdBuff, ret.image,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1,
				0, 1
			}
		);

		uint32_t width = uwidth, height = uheight;
		for (uint32_t level = 1; level < mipLevels; level++)
		{
			VkImageBlit blit{};
			blit.srcSubresource = VkImageSubresourceLayers
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				level - 1,
				0, 1
			};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { static_cast<int32_t>(width), static_cast<int32_t>(height), 1 };

			width >>= 1;
			if (width == 0)
				width = 1;

			height >>= 1;
			if (height == 0)
				height = 1;

			blit.dstSubresource = VkImageSubresourceLayers
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				level,
				0, 1
			};
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { static_cast<int32_t>(width), static_cast<int32_t>(height), 1 };

			vkCmdBlitImage(cmdBuff,
				ret.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			/* BARRIER: TRANSFER_DST -> TRANSFER_SRC */
			image_barrier(cmdBuff, ret.image,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange
				{
					VK_IMAGE_ASPECT_COLOR_BIT,
					level, 1,
					0, 1
				});
		}

		/* BARRIER: TRANSFER_SRC -> SHADER_READ */
		image_barrier(cmdBuff, ret.image,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VkImageSubresourceRange
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, mipLevels,
				0, 1
			});

		if (const auto& res = vkEndCommandBuffer(cmdBuff); res != VK_SUCCESS)
		{
			throw Error("VK: vkEndCommandBuffer() failed during texture upload. err: %s",
				to_string(res).c_str());
		}

		Fence uploadComplete = create_fence(aContext);

		VkSubmitInfo subInfo{};
		subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		subInfo.commandBufferCount = 1;
		subInfo.pCommandBuffers = &cmdBuff;

		if (const auto& res = vkQueueSubmit(aContext.graphicsQueue, 1, &subInfo, *uploadComplete); res != VK_SUCCESS)
		{
			throw Error("VK: vkQueueSubmit() failed during texture upload. err: %s",
				to_string(res).c_str());
		}

		if (const auto& res = vkWaitForFences(aContext.device, 1, &*uploadComplete, VK_TRUE, std::numeric_limits<uint32_t>::max());
			res != VK_SUCCESS)
		{
			throw Error("VK: vkWaitForFences() failed after texture upload. err: %s",
				to_string(res).c_str());
		}

		vkFreeCommandBuffers(aContext.device, aCmdPool, 1, &cmdBuff);

		return ret;
	}

	Image create_image_texture2d(Allocator const& aAllocator, std::uint32_t aWidth, std::uint32_t aHeight, VkFormat aFormat, VkImageUsageFlags aUsage)
	{
		/* Compute the count of mip levels based upon the given image dimensions */
		/* The mip levels are each a quarter of the area of the preceding level with
			the highest level being aWidth x aHeight and the lowest being 1 x 1 */
		auto const mipLevels = compute_mip_level_count(aWidth, aHeight);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = aFormat;
		imageInfo.extent.width = aWidth;
		imageInfo.extent.height = aHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = aUsage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VkImage image = VK_NULL_HANDLE;
		VmaAllocation alloc = VK_NULL_HANDLE;
		if (const auto& res = vmaCreateImage(*aAllocator, &imageInfo, &allocInfo, &image, &alloc, nullptr);
			res != VK_SUCCESS)
		{
			throw Error("VK: vmaCreateImage() failed. err: %s",
				to_string(res).c_str());
		}

		return Image(*aAllocator, image, alloc);
	}

	std::uint32_t compute_mip_level_count(std::uint32_t aWidth, std::uint32_t aHeight)
	{
		std::uint32_t const bits = aWidth | aHeight;
		std::uint32_t const leadingZeros = countl_zero_(bits);
		return 32 - leadingZeros;
	}
}
