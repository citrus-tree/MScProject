#include "TextureUtilities.hpp"

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"

labutils::ImageView Renderer::CreateImageView(const Environment* environment, VkImage image, VkFormat format)
{
	using namespace labutils;

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.components = VkComponentMapping{};
	viewInfo.subresourceRange = VkImageSubresourceRange
	{
		VK_IMAGE_ASPECT_COLOR_BIT,
		0, VK_REMAINING_MIP_LEVELS,
		0, 1
	};

	VkImageView view = VK_NULL_HANDLE;
	if (auto const res = vkCreateImageView(environment->Window().device, &viewInfo, nullptr, &view); VK_SUCCESS != res)
	{
		throw Error("VK: vkCreateImageView() failed to create a texture image view. err: %s",
			to_string(res).c_str());
	}

	return ImageView(environment->Window().device, view);
}

void Renderer::CmdPrimeImageForWrite(Environment* environment, const lut::Image* image, bool isDepth)
{
	/* BARRIER: undefined -> colour attachment */
	lut::image_barrier(*environment->CurrentCmdBuffer(), **image,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		(isDepth) ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VkImageSubresourceRange
		{
			static_cast<VkImageAspectFlags>((isDepth) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
			0, 1,
			0, 1
		});
}

void Renderer::CmdPrimeImageForRead(Environment* environment, const lut::Image* image, bool isDepth)
{
	/* BARRIER: undefined -> shader read */
	lut::image_barrier(*environment->CurrentCmdBuffer(), **image,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VkImageSubresourceRange
		{
			static_cast<VkImageAspectFlags>((isDepth) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
			0, 1,
			0, 1
		});
}

void Renderer::CmdTransitionForWrite(Environment* environment, const lut::Image* image, bool isDepth)
{
	/* BARRIER: shader read -> colour attachment */
	lut::image_barrier(*environment->CurrentCmdBuffer(), **image,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		(isDepth) ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VkImageSubresourceRange
		{
			static_cast<VkImageAspectFlags>((isDepth) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
			0, 1,
			0, 1
		});
}

void Renderer::CmdTransitionForRead(Environment* environment, const lut::Image* image, bool isDepth)
{
	/* BARRIER: colour attachment -> shader read */
	lut::image_barrier(*environment->CurrentCmdBuffer(), **image,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		(isDepth) ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VkImageSubresourceRange
		{
			static_cast<VkImageAspectFlags>((isDepth) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
			0, 1,
			0, 1
		});
}
