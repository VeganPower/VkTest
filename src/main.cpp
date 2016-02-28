//
#include "config.hpp"
#include <vulkan/vulkan.h>

#include <stdio.h>
#include <cassert>
#include <vector>

#include "error.hpp"
#include "win32.hpp"
#include <iostream>

#define STD_CALL __stdcall
bool check_flag(uint32_t flag, uint32_t bit)
{
    return (flag & bit) == bit;
}

struct SwapChain
{
    HINSTANCE instance;
    HWND window;
    VkPhysicalDevice gpu;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surface_format;
    VkSwapchainKHR swap_chain;
    VkQueue present_queue;
    uint32_t queue_family_idx;
};

struct DrawCommandBuffer
{
    VkQueue draw_queue;
    uint32_t queue_family_idx;
};

VkResult create_device(VkPhysicalDevice gpu, uint32_t graphics_queue, uint32_t present_queue, VkDevice* out_device)
{
    const float queuePriority = 1.0f;
    const VkDeviceQueueCreateInfo requested_queues[] =
    {
        {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            NULL,
            0,
            graphics_queue,
            1,
            &queuePriority
        },
        {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            NULL,
            0,
            present_queue,
            1,
            &queuePriority
        }
    };
    uint32_t requested_queue_count = (graphics_queue == present_queue) ? 1 : 2;


    VkDeviceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.queueCreateInfoCount = requested_queue_count;
    info.pQueueCreateInfos = requested_queues;
    info.enabledLayerCount = 0;
    info.ppEnabledLayerNames = nullptr;
    info.enabledExtensionCount = 0;
    info.ppEnabledExtensionNames = nullptr;
    info.pEnabledFeatures = nullptr;
    VK_THROW(vkCreateDevice(gpu, &info, nullptr, out_device));
    return VK_SUCCESS;
}

VkResult create_surface(VkInstance vk_instance, VkPhysicalDevice gpu, VkDevice* out_device, DrawCommandBuffer* out_draw_command_buffer, SwapChain* out_swap_chain)
{
    if ((out_swap_chain == nullptr) || (out_draw_command_buffer == nullptr))
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = out_swap_chain->instance;
    surfaceCreateInfo.hwnd = out_swap_chain->window;
    VK_THROW(vkCreateWin32SurfaceKHR(vk_instance, &surfaceCreateInfo, NULL, &(out_swap_chain->surface)));


    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, nullptr);
    assert(queue_count > 0);
    std::vector<VkBool32> support_presentable_swap_chain(queue_count);
    std::vector<VkQueueFamilyProperties> properties(queue_count);

    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, properties.data());
    for (uint32_t qidx = 0; qidx < queue_count; ++qidx)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, qidx, out_swap_chain->surface, &(support_presentable_swap_chain[qidx]));
    }
    uint32_t graphics_queue = UINT32_MAX;
    uint32_t swap_chain_queue = UINT32_MAX;
    for (uint32_t qidx = 0; qidx < queue_count; ++qidx)
    {
        if (check_flag(properties[qidx].queueFlags, VK_QUEUE_GRAPHICS_BIT) && properties[qidx].queueCount > 0)
        {
            graphics_queue = qidx;
            if (support_presentable_swap_chain[qidx])
            {
                swap_chain_queue = qidx;
                break;
            }
        }
    }
    if (swap_chain_queue == UINT32_MAX)   // Can't find a graphic queue that also support swap chain. Select two different queue.
    {
        for (uint32_t qidx = 0; qidx < queue_count; ++qidx)
        {
            if (support_presentable_swap_chain[qidx] && (properties[qidx].queueCount > 0))
            {
                swap_chain_queue = qidx;
                break;
            }
        }
    }

    // Generate error if could not find both a graphics and a present queue
    if ((graphics_queue == UINT32_MAX) || (swap_chain_queue == UINT32_MAX))
    {
        vkDestroySurfaceKHR(vk_instance, out_swap_chain->surface, nullptr);
        out_swap_chain->surface = nullptr;
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    uint32_t format_count;
    VK_THROW(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, out_swap_chain->surface, &format_count, NULL));

    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    VK_THROW(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, out_swap_chain->surface, &format_count, surface_formats.data()));

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format. Otherwise, at least one
    // supported format will be returned
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        out_swap_chain->surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
        out_swap_chain->surface_format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    }
    else
    {
        assert(format_count > 0);
        out_swap_chain->surface_format = surface_formats[0];
    }

    VK_THROW(create_device(gpu, graphics_queue, swap_chain_queue, out_device));

    vkGetDeviceQueue(*out_device, graphics_queue, 0, &out_draw_command_buffer->draw_queue);
    vkGetDeviceQueue(*out_device, swap_chain_queue, 0, &out_swap_chain->present_queue);
    out_draw_command_buffer->queue_family_idx = graphics_queue;
    out_swap_chain->queue_family_idx = swap_chain_queue;
    out_swap_chain->gpu = gpu;

    return VK_SUCCESS;
}

VkResult init_Vulkan(VkInstance& vk_instance, VkDevice* out_device, DrawCommandBuffer* out_draw_command_buffer, SwapChain* out_swap_chain)
{
    if ((out_device == nullptr) || (out_swap_chain == nullptr))
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = "VT Test";
    app_info.applicationVersion = 0x00000001;
    app_info.pEngineName = "Noengine";
    app_info.engineVersion = 0x00;
    app_info.apiVersion = VK_MAKE_VERSION(1, 0, 2);

    std::vector<const char *> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
    enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = nullptr;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = 0;
    instance_info.ppEnabledLayerNames = nullptr;
    instance_info.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instance_info.ppEnabledExtensionNames = enabledExtensions.data();

    VK_THROW(vkCreateInstance(&instance_info, nullptr, &vk_instance)); // Create Vulkan library instance

    out_swap_chain->instance = GetModuleHandle(NULL);
    out_swap_chain->window = setup_window(out_swap_chain->instance, 800, 600, "vk_test");

    uint32_t gpu_count = 0;
    // Get number of available physical devices
    VK_THROW(vkEnumeratePhysicalDevices(vk_instance, &gpu_count, nullptr));
    assert(gpu_count > 0);

    std::vector<VkPhysicalDevice> physicalDevices(gpu_count); // Enumerate devices
    VK_THROW(vkEnumeratePhysicalDevices(vk_instance, &gpu_count, physicalDevices.data()));
    bool surface_created = false;
    VkPhysicalDeviceProperties gpu_properties;
    VkPhysicalDeviceFeatures gpu_feature;
    for (uint32_t gidx = 0; gidx < gpu_count; ++gidx)
    {
        VkPhysicalDevice gpu = physicalDevices[gidx];
        vkGetPhysicalDeviceProperties(gpu, &gpu_properties);
        std::cout << gidx << "Driver Version: " << gpu_properties.driverVersion << "\n";
        std::cout << gidx << "Device Name:    " << gpu_properties.deviceName << "\n";
        std::cout << gidx << "Device Type:    " << gpu_properties.deviceType << "\n";
        std::cout << gidx << "API Version:    " << VK_VERSION_MAJOR(gpu_properties.apiVersion) << "." << VK_VERSION_MINOR(gpu_properties.apiVersion) << "." << VK_VERSION_PATCH(gpu_properties.apiVersion) << "\n";

        // we can choose a gpu based on the features here.
        vkGetPhysicalDeviceFeatures(gpu, &gpu_feature);

        VkResult err = create_surface(vk_instance, gpu, out_device, out_draw_command_buffer, out_swap_chain);
        if (err == VK_SUCCESS)
        {
            surface_created = true;
            break;
        }
    }
    if (!surface_created)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return VK_SUCCESS;
}

VkResult create_command_pool(VkDevice device, uint32_t queue_family_idx, VkCommandPool* out_command_pool)
{
    VkCommandPoolCreateInfo command_pool_info = {};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = nullptr;
    command_pool_info.queueFamilyIndex = queue_family_idx;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    return vkCreateCommandPool(device, &command_pool_info, nullptr, out_command_pool);
}

VkResult setup(VkDevice device, VkCommandPool command_pool, SwapChain& swap_chain, uint32_t *width, uint32_t *height)
{
    VkCommandBuffer setup_command_buffer = VK_NULL_HANDLE;  // command buffer used for setup
    VkCommandBufferAllocateInfo command_buffer_allocate_info;
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &setup_command_buffer);

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(setup_command_buffer, &command_buffer_begin_info);

    // SWAP CHAIN
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(swap_chain.gpu, swap_chain.surface, &present_mode_count, nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(swap_chain.gpu, swap_chain.surface, &present_mode_count, present_modes.data());

    // Try to use mailbox mode
    // Low latency and non-tearing
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < present_mode_count; i++)
    {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
        {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    VkSurfaceCapabilitiesKHR surface_cap;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(swap_chain.gpu, swap_chain.surface, &surface_cap);

    VkExtent2D swapchainExtent = {};
    // width and height are either both -1, or both not -1.
    if (surface_cap.currentExtent.width == -1)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = *width;
        swapchainExtent.height = *height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surface_cap.currentExtent;
        *width = surface_cap.currentExtent.width;
        *height = surface_cap.currentExtent.height;
    }

    // Determine the number of images
    uint32_t desired_number_of_swapchain_images = surface_cap.minImageCount + 1;
    if ((surface_cap.maxImageCount > 0) && (desired_number_of_swapchain_images > surface_cap.maxImageCount))
    {
        desired_number_of_swapchain_images = surface_cap.maxImageCount;
    }

    VkSurfaceTransformFlagsKHR pre_transform;
    if (check_flag(surface_cap.supportedTransforms, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR))
    {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        pre_transform = surface_cap.currentTransform;
    }
	/*
    VkSwapchainCreateInfoKHR swapchain_creation_info = {};
    swapchain_creation_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_creation_info.pNext = NULL;
    swapchain_creation_info.surface = surface;
    swapchain_creation_info.minImageCount = desired_number_of_swapchain_images;
    swapchain_creation_info.imageFormat = colorFormat;
    swapchain_creation_info.imageColorSpace = colorSpace;
    swapchain_creation_info.imageExtent = { swapchainExtent.width, swapchainExtent.height };
    swapchain_creation_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_creation_info.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchain_creation_info.imageArrayLayers = 1;
    swapchain_creation_info.queueFamilyIndexCount = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_creation_info.queueFamilyIndexCount = 0;
    swapchain_creation_info.pQueueFamilyIndices = NULL;
    swapchain_creation_info.presentMode = swapchainPresentMode;
    swapchain_creation_info.oldSwapchain = VK_NULL_HANDLE;
    swapchain_creation_info.clipped = true;
    swapchain_creation_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    vkCreateSwapchainKHR(device, &swapchain_creation_info, nullptr, &swap_chain.swap_chain);


    std::vector<VkImage> swap_chain_images(desired_number_of_swapchain_images);
    vkGetSwapchainImagesKHR(device, swapChain, &desired_number_of_swapchain_images, swap_chain_images.data());
*/
/*
    assert(!err);

    buffers = (SwapChainBuffer*)malloc(sizeof(SwapChainBuffer)*imageCount);
    assert(buffers);
    //buffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = NULL;
        colorAttachmentView.format = colorFormat;
        colorAttachmentView.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };
        colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags = 0;

        buffers[i].image = swapchainImages[i];

        vkTools::setImageLayout(
            cmdBuffer,
            buffers[i].image,
            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        colorAttachmentView.image = buffers[i].image;

        err = vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view);
        assert(!err);
    }
*/
    vkEndCommandBuffer(setup_command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &setup_command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    vkQueueSubmit(swap_chain.present_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(swap_chain.present_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &setup_command_buffer);
    return VK_SUCCESS;
}

//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
int main(int argc, char *argv[])
{
    VkInstance vk_instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    SwapChain swap_chain;
    DrawCommandBuffer command_buffer;
    VkCommandPool swap_chain_command_pool = VK_NULL_HANDLE;
    VkCommandPool draw_command_pool = VK_NULL_HANDLE;

    if (VkResult err = init_Vulkan(vk_instance, &device, &command_buffer, &swap_chain))
    {
        print_vk_error_code("Unable to initialize Vulkan: ", err);
        return 1;
    }

    // Swap chain command pool
    create_command_pool(device, swap_chain.queue_family_idx, &swap_chain_command_pool);
    if (swap_chain.queue_family_idx == command_buffer.queue_family_idx)
    {
        draw_command_pool = swap_chain_command_pool;
    }
    else
    {
        create_command_pool(device, command_buffer.queue_family_idx, &draw_command_pool);
    }
    uint32_t w = 800;
    uint32_t h = 600;
    setup(device, swap_chain_command_pool, swap_chain, &w, &h);

    //while (true)
    {
        // clear screen
        // swap chain
    }

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(vk_instance, nullptr);

    return 0;
}