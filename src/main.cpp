#include <Windows.h>
#include <exception>
#include <iostream>
#include <unordered_map>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

#include "helpers.h"

#if defined _DEBUG
#define ENABLE_VULKAN_VALIDATION
#endif


//simple window procedure
LRESULT CALLBACK window_proc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	if (uMsg == WM_DESTROY) {
		std::cout << "WM_DESTROY\n";
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}


//register window class helper
ATOM register_wnd_class(HINSTANCE hInstance, LPCWSTR class_name) {
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_OWNDC;
	wcex.lpfnWndProc = &window_proc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = class_name;
	wcex.hIconSm = NULL;
	return ::RegisterClassExW(&wcex);
}


//simple HWND wrapper
class window {
public:
	window(ATOM wnd_class, LPCWSTR title, HINSTANCE hInstance) : hWnd(::CreateWindowExW(0, (LPCWSTR) (WORD) wnd_class, title,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 500, 500, NULL, NULL, hInstance, nullptr))
	{
		assert(hWnd != NULL);
	}

	operator HWND() const {
		return hWnd;
	}

	~window() {
		assert(hWnd != NULL);
		::DestroyWindow(hWnd);
		hWnd = NULL;
	}

private:
	HWND hWnd = NULL;
};


VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	static const std::unordered_map<vk::DebugUtilsMessageSeverityFlagBitsEXT, std::string> levels = {
		{vk::DebugUtilsMessageSeverityFlagBitsEXT::eError, "[err]"},
		{vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning, "[warn]"},
		{vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo, "[info]"},
		{vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose, "[trace]"}
	};
	const auto lvl_name = levels.at((vk::DebugUtilsMessageSeverityFlagBitsEXT) messageSeverity);

	std::cerr << lvl_name << ": " << pCallbackData->pMessage << "\n";

	return VK_FALSE;
}

static auto debug_create_info = vk::DebugUtilsMessengerCreateInfoEXT({},
	vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
	vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
	vulkan_debug_callback);


vk::UniqueInstance create_vulkan_instance() {
	const auto application_info = vk::ApplicationInfo(
		"test", VK_MAKE_VERSION(0, 0, 1),
		"test", VK_MAKE_VERSION(0, 0, 1),
		VK_API_VERSION_1_1
	);

#if defined ENABLE_VULKAN_VALIDATION
	static constexpr std::array<const char *, 1> layers = { "VK_LAYER_LUNARG_standard_validation" };
#else
	static constexpr std::array<const char *, 0> layers = { };
#endif
	static constexpr std::array<const char *, 3> extensions = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

	return vk::createInstanceUnique(vk::InstanceCreateInfo()
		.setPApplicationInfo(&application_info)
		.setEnabledLayerCount((uint32_t) layers.size())
		.setPpEnabledLayerNames(layers.data())
		.setEnabledExtensionCount((uint32_t) extensions.size())
		.setPpEnabledExtensionNames(extensions.data())
		.setPNext((vk::DebugUtilsMessengerCreateInfoEXT*) &debug_create_info)
	);
}

vk::UniqueDevice create_vulkan_device(const vk::PhysicalDevice & phy_device, uint32_t queue_family) {
	constexpr std::array<float, 1> queue_priorities = {1.0f};
	const std::array<vk::DeviceQueueCreateInfo, 1> queue_ci{
		vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queue_family, (uint32_t) queue_priorities.size(), queue_priorities.data())
	};

	static constexpr std::array<const char *, 1> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	return phy_device.createDeviceUnique(vk::DeviceCreateInfo()
		.setQueueCreateInfoCount((uint32_t) queue_ci.size())
		.setPQueueCreateInfos(queue_ci.data())
		.setEnabledLayerCount(0)
		.setPpEnabledLayerNames(nullptr)
		.setEnabledExtensionCount((uint32_t) extensions.size())
		.setPpEnabledExtensionNames(extensions.data())
		.setPEnabledFeatures(nullptr));
}

vk::UniqueRenderPass create_renderpass(const vk::Device & device, vk::Format attachment_format) {
	const auto color_attachment_ref = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal); //index in renderpass_attachment_descriptions

	const std::array<vk::SubpassDescription, 1> subpasses = {
		vk::SubpassDescription(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics,
		0, nullptr,
		1, &color_attachment_ref,
		nullptr, nullptr, 0, nullptr)
	};

	const std::array<vk::AttachmentDescription, 1> attachment_descriptions = {
		vk::AttachmentDescription(vk::AttachmentDescriptionFlags(), attachment_format, vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, //stencil
		vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)
	};

	return device.createRenderPassUnique(vk::RenderPassCreateInfo(
		vk::RenderPassCreateFlags(),
		(uint32_t) attachment_descriptions.size(), attachment_descriptions.data(),
		(uint32_t) subpasses.size(), subpasses.data(), 0, nullptr
	));
}

struct swapchain_resources {
	std::vector<vk::UniqueImageView> attachment_views;
	std::vector<vk::UniqueFramebuffer> framebuffers;
	std::vector<vk::CommandBuffer> command_buffers;

	std::vector<vk::UniqueSemaphore> acquire_semaphores; //signalled when presentation finished reading from the image. must wait for this semaphore before executing draw commands
	std::vector<vk::UniqueSemaphore> completed_semaphores; //signalled when command buffer finished execution
	std::vector<vk::UniqueFence> cmd_fences;
};


void draw_image(const vk::Device & device, const vk::Queue & queue,
	const vk::SwapchainKHR & swapchain, const vk::Extent2D & swapchain_extent, const swapchain_resources & swr, const vk::RenderPass & renderpass, int frame_num)
{
	const auto [acquire_res, image_index] = device.acquireNextImageKHR(swapchain, UINT64_MAX, swr.acquire_semaphores[frame_num % 2].get(), nullptr);
	std::cout << "acquireNextImageKHR result " << acquire_res << ", image index = " << image_index << "\n";

	const vk::CommandBuffer cmd = swr.command_buffers[frame_num % 2];
	device.waitForFences(swr.cmd_fences[frame_num % 2].get(), VK_TRUE, UINT32_MAX);

	cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	static const std::array<vk::ClearValue, 1> clear_colors = { vk::ClearColorValue(std::array<float, 4>{0.0, 1.0f, 0.0, 0.0f}) };

	const vk::Rect2D render_area(vk::Offset2D(0, 0), swapchain_extent);

	cmd.beginRenderPass(vk::RenderPassBeginInfo(renderpass, swr.framebuffers[image_index].get(), render_area,
		(uint32_t) clear_colors.size(), clear_colors.data()), vk::SubpassContents::eInline);

	cmd.endRenderPass();
	cmd.end();

	const vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	device.resetFences(swr.cmd_fences[frame_num % 2].get());
	queue.submit(vk::SubmitInfo(
		1, &swr.acquire_semaphores[frame_num % 2].get(), &wait_stage,
		1, &cmd,
		1, &swr.completed_semaphores[frame_num % 2].get()),
		swr.cmd_fences[frame_num % 2].get());

	[[maybe_unused]] const auto present_res = queue.presentKHR(vk::PresentInfoKHR(1, &swr.completed_semaphores[frame_num % 2].get(), 1, &swapchain, &image_index));
	std::cout << "presentKHR result " << present_res << "\n";
}


void test_blit(const vk::Device & device, const vk::CommandPool & command_pool, const vk::Queue & queue, const vk::Image & source, const vk::Image & dest, sizei image_size) {
	for (int i = 0; i < 10; i++) {
		const auto cmd = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(command_pool, vk::CommandBufferLevel::ePrimary, 1));
		const auto fence = device.createFenceUnique(vk::FenceCreateInfo());

		cmd[0].begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		if (i == 0) {
			append_image_transition(cmd[0], source, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal);
			append_image_transition(cmd[0], dest, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		}
		append_image_blit(cmd[0], source, image_size, dest, image_size);
		cmd[0].end();

		queue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd[0], 0, nullptr), fence.get());
		device.waitForFences(fence.get(), VK_TRUE, UINT64_MAX);
	}
}

void show_window(HINSTANCE hInstance, const vk::Instance & instance, const vk::PhysicalDevice & phy_device, const vk::Device & device,
	uint32_t queue_family, const vk::Queue & queue)
{
	const auto command_pool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), queue_family));

	const auto wnd_class = register_wnd_class(::GetModuleHandle(NULL), L"vulkan");
	const window image_window(wnd_class, L"Vulkan", hInstance);

	const vk::UniqueSurfaceKHR surface = instance.createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(), hInstance, image_window));

	const auto [swapchain, surface_format, surface_extent] = create_swapchain(phy_device, device, surface.get(), queue_family);

	const auto swapchain_images = device.getSwapchainImagesKHR(swapchain.get());
	assert(swapchain_images.size() == 2);

	const auto renderpass = create_renderpass(device, surface_format);

	swapchain_resources swr;

	swr.command_buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(
		command_pool.get(), vk::CommandBufferLevel::ePrimary, (uint32_t) swapchain_images.size()));

	swr.attachment_views.resize(swapchain_images.size());
	swr.framebuffers.resize(swapchain_images.size());
	for (int i = 0; i < swapchain_images.size(); i++) {
		swr.attachment_views[i] = device.createImageViewUnique(vk::ImageViewCreateInfo(
			vk::ImageViewCreateFlags(), swapchain_images[i], vk::ImageViewType::e2D, surface_format,
			vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));

		const std::array<vk::ImageView, 1> fb_attachments = { swr.attachment_views[i].get() };

		swr.framebuffers[i] = device.createFramebufferUnique(vk::FramebufferCreateInfo(vk::FramebufferCreateFlags(),
			renderpass.get(), (uint32_t) fb_attachments.size(), fb_attachments.data(), surface_extent.width, surface_extent.height, 1));
	}

	swr.acquire_semaphores.resize(swapchain_images.size());
	swr.completed_semaphores.resize(swapchain_images.size());
	swr.cmd_fences.resize(swapchain_images.size());
	for (int i = 0; i < swapchain_images.size(); i++) {
		swr.acquire_semaphores[i] = device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
		swr.completed_semaphores[i] = device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
		swr.cmd_fences[i] = device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}

	draw_image(device, queue, swapchain.get(), surface_extent, swr, renderpass.get(), 0);
	draw_image(device, queue, swapchain.get(), surface_extent, swr, renderpass.get(), 1);

	{
		assert(device);
		std::vector<vk::Fence> fences(swr.cmd_fences.size());
		std::transform(swr.cmd_fences.begin(), swr.cmd_fences.end(), fences.begin(), [](const auto & unique) { return unique.get(); });
		device.waitForFences(fences, VK_TRUE, UINT64_MAX);
	}

	device.waitIdle();
}


int main(int argc, char * argv[]) {
	try {
		int gpu_index = 0;
		if (argc > 1) {
			const std::string gpu_arg(argv[1]);
			try {
				gpu_index = std::stoi(gpu_arg);
			} catch (...) {
				std::cerr << "can't parse gpu argument: " << gpu_arg << "\n";
			}
		}
		const HINSTANCE hInstance = GetModuleHandle(NULL);
		const auto instance = create_vulkan_instance();

		const auto debug_messanger = instance->createDebugUtilsMessengerEXTUnique(debug_create_info, nullptr,
			vk::DispatchLoaderDynamic(instance.get(), ::vkGetInstanceProcAddr));

		const auto phy_devices = instance->enumeratePhysicalDevices();
		const auto phy_device = phy_devices[gpu_index];
		const auto props = phy_device.getProperties();
		const auto mem_props = phy_device.getMemoryProperties();
		std::cout << "Using device: " << props.deviceName << "\n";

		const auto queue_properties = phy_device.getQueueFamilyProperties();
		const uint32_t queue_family = 0;

		const auto device = create_vulkan_device(phy_device, queue_family);

		const auto queue = device->getQueue(queue_family, 0);

		const auto format = vk::Format::eR8Unorm;
		const auto image_size = sizei{500, 500};

		const auto command_pool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), queue_family));

		show_window(hInstance, instance.get(), phy_device, device.get(), queue_family, queue);

		const auto [source, source_mem] = create_device_backed_image(device.get(), phy_device.getMemoryProperties(), format, image_size.width, image_size.height,
			vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst);

		const auto [dest, dest_mem] = create_device_backed_image(device.get(), phy_device.getMemoryProperties(), format, image_size.width, image_size.height,
			vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst);

		test_blit(device.get(), command_pool.get(), queue, source.get(), dest.get(), image_size);

	} catch (std::exception & e) {
		std::cerr << "exception in main(): " << e.what() << "\n";
		assert(false);
		return EXIT_FAILURE;
	} catch (...) {
		std::cerr << "unknown exception in main()\n";
		assert(false);
		return EXIT_FAILURE;
	}

	std::cout << "exit clean\n";
	return EXIT_SUCCESS;
}