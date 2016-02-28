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
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surface_format;
    VkQueue present_queue;
};

struct DrawCommandBuffer
{
    VkQueue draw_queue;
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

//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
int main(int argc, char *argv[])
{
    VkInstance vk_instance;
    VkDevice device;
    SwapChain swap_chain;
    DrawCommandBuffer command_buffer;

    if (VkResult err = init_Vulkan(vk_instance, &device, &command_buffer, &swap_chain))
    {
        print_vk_error_code("Unable to initialize Vulkan: ", err);
        return 1;
    }

    //


	//while (true)
	{
		// clear screen
		// swap chain
	}

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(vk_instance, nullptr);

    return 0;
}