#ifndef HELPERS_H
#define HELPERS_H

#include <vector>
#include <vulkan/vulkan.hpp>

struct sizei {
	int width, height;
};


struct find_memory_type_result {
	uint32_t type_index;
	uint32_t heap_index;
};

inline std::vector<find_memory_type_result> find_memory_type(const vk::PhysicalDeviceMemoryProperties & mem_props, uint32_t typeFilter, vk::MemoryPropertyFlags flags) {
	std::vector<find_memory_type_result> r;

	for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && ((mem_props.memoryTypes[i].propertyFlags & flags) == flags)) {
			r.emplace_back(find_memory_type_result{ i, mem_props.memoryTypes[i].heapIndex });
		}
	}

	return r;
}


std::tuple<vk::UniqueImage, vk::UniqueDeviceMemory> create_device_backed_image(const vk::Device & device, const vk::PhysicalDeviceMemoryProperties & mem_props,
	const vk::Format format, int width, int height, const vk::ImageUsageFlags usage)
{
	auto image = device.createImageUnique(vk::ImageCreateInfo(vk::ImageCreateFlags(),
		vk::ImageType::e2D, format, vk::Extent3D(width, height, 1), 1, 1,
		vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
		usage, vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined));

	const auto mem_req = device.getImageMemoryRequirements(image.get());

	const auto mem_types = find_memory_type(mem_props, mem_req.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
	if (mem_types.size() == 0)

		throw std::runtime_error("Failed to find suitable memory type");
	auto mem = device.allocateMemoryUnique(vk::MemoryAllocateInfo(mem_req.size, mem_types[0].type_index));
	device.bindImageMemory(image.get(), mem.get(), 0);

	return {std::move(image), std::move(mem)};
}

std::tuple<vk::UniqueSwapchainKHR, vk::Format, vk::Extent2D> create_swapchain(const vk::PhysicalDevice & phy_device, const vk::Device & device, const vk::SurfaceKHR & surface, uint32_t queue_family) {
	//standard capabilities check
	const vk::SurfaceCapabilitiesKHR capabilities = phy_device.getSurfaceCapabilitiesKHR(surface);

	const std::vector<vk::SurfaceFormatKHR> formats = phy_device.getSurfaceFormatsKHR(surface);
	const std::vector<vk::PresentModeKHR> present_modes = phy_device.getSurfacePresentModesKHR(surface);

	const auto supported = phy_device.getSurfaceSupportKHR(queue_family, surface);
	assert(supported);

	//pick anything
	const auto image_count = capabilities.minImageCount;
	const auto format = formats.at(0).format;
	const auto color_space = formats.at(0).colorSpace;
	const auto present_mode = present_modes.at(0);
	const auto extent = capabilities.currentExtent;

	return { device.createSwapchainKHRUnique(vk::SwapchainCreateInfoKHR(vk::SwapchainCreateFlagsKHR(), surface, image_count, format,
		color_space, extent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr,
		vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque,
		present_mode, VK_FALSE, nullptr)), format, extent};
}


inline void append_image_barrier(const vk::CommandBuffer & cmd, const vk::Image & image,
	vk::PipelineStageFlags src_stage, vk::PipelineStageFlags dst_stage,
	vk::AccessFlags src_access, vk::AccessFlags dst_access,
	const vk::ImageLayout & initial_layout, const vk::ImageLayout & final_layout,
	uint32_t base_layer = 0, uint32_t layer_count = 1)
{
	cmd.pipelineBarrier(src_stage, dst_stage,
		vk::DependencyFlagBits::eByRegion, nullptr, nullptr,
		vk::ImageMemoryBarrier(src_access, dst_access, initial_layout, final_layout,
		0, 0, image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, base_layer, layer_count)));
}


inline void append_image_transition(const vk::CommandBuffer & cmd, const vk::Image & image,
	const vk::ImageLayout & initial_layout, const vk::ImageLayout & final_layout, int base_layer = 0, int layer_count = 1)
{
	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
		vk::DependencyFlagBits::eByRegion, nullptr, nullptr,
		vk::ImageMemoryBarrier(vk::AccessFlags(), vk::AccessFlags(), initial_layout, final_layout,
			0, 0, image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, base_layer, layer_count)));
}


inline void append_image_blit(const vk::CommandBuffer & cmd, const vk::Image & source, const sizei & source_size,
	const vk::Image & dest, const sizei & dest_size)
{
	const auto region = vk::ImageBlit(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
		{ vk::Offset3D{0, 0, 0}, vk::Offset3D{source_size.width, source_size.height, 1} },
		vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
		{ vk::Offset3D{0, 0, 0}, vk::Offset3D{dest_size.width, dest_size.height, 1} });
	cmd.blitImage(source, vk::ImageLayout::eTransferSrcOptimal, dest, vk::ImageLayout::eTransferDstOptimal, region, vk::Filter::eLinear);
}

#endif //HELPERS_H