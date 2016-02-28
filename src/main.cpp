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
    return (flag & bit)!= bit;
}

VkResult create_device(VkInstance vulkan, VkPhysicalDevice gpu, VkDevice* out_device)
{
    VkDeviceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.queueCreateInfoCount = 0;
    info.pQueueCreateInfos = nullptr;
    info.enabledLayerCount = 0;
    info.ppEnabledLayerNames = nullptr;
    info.enabledExtensionCount = 0;
    info.ppEnabledExtensionNames = nullptr;
    info.pEnabledFeatures = nullptr;
	VK_THROW(vkCreateDevice(gpu, &info, nullptr, out_device));
	return VK_SUCCESS;
}

VkResult choose_device(VkInstance vulkan, VkPhysicalDevice* selected_gpu)
{
    uint32_t gpu_count = 0;
    // Get number of available physical devices
    VK_THROW(vkEnumeratePhysicalDevices(vulkan, &gpu_count, nullptr));
    assert(gpu_count > 0);

    std::vector<VkPhysicalDevice> physicalDevices(gpu_count); // Enumerate devices
    VK_THROW(vkEnumeratePhysicalDevices(vulkan, &gpu_count, physicalDevices.data()));

    uint32_t selected_gpu_idx = 0;
    uint32_t selected_queue = 0;
    bool found = false;
    VkPhysicalDeviceProperties gpu_properties;
	VkPhysicalDeviceFeatures gpu_feature;
    for (auto gpu : physicalDevices )
    {
        vkGetPhysicalDeviceProperties(gpu, &gpu_properties);
        std::cout << "Driver Version: " << gpu_properties.driverVersion << "\n";
        std::cout << "Device Name:    " << gpu_properties.deviceName << "\n";
        std::cout << "Device Type:    " << gpu_properties.deviceType << "\n";
        std::cout << "API Version:    " << VK_VERSION_MAJOR(gpu_properties.apiVersion) << "." << VK_VERSION_MINOR(gpu_properties.apiVersion) << "." << VK_VERSION_PATCH(gpu_properties.apiVersion) << "\n";

		// we can choose a gpu based on the features here.
		vkGetPhysicalDeviceFeatures(gpu, &gpu_feature);

        uint32_t properties_count;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &properties_count, nullptr);
        std::vector<VkQueueFamilyProperties> properties(properties_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &properties_count, properties.data());
        for (auto prop : properties)
        {
            if (check_flag(prop.queueFlags, VK_QUEUE_GRAPHICS_BIT) && prop.queueCount > 0)
            {
                found = true;
                break;
            }
            selected_queue++;
        }
        if (found)
        {
            break;
        }
        selected_gpu_idx++;
    }
	if (!found)
		return VK_ERROR_FEATURE_NOT_PRESENT;
	*selected_gpu = physicalDevices[selected_gpu_idx];
	return VK_SUCCESS;
}

VkResult create_surface(VkInstance vk_instance, VkPhysicalDevice gpu, HINSTANCE instance, HWND window)
{
    VkSurfaceKHR surface;
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = instance;
    surfaceCreateInfo.hwnd = window;
    VK_THROW(vkCreateWin32SurfaceKHR(vk_instance, &surfaceCreateInfo, NULL, &surface));


    VkFormat colorFormat;
    uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, NULL);

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, surfaceFormats.data());
/*
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format. Otherwise, at least one
    // supported format will be returned
    if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
        colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    else {
        assert(formatCount >= 1);
        colorFormat = surfaceFormats[0].format;
    }
    colorSpace = surfaceFormats[0].colorSpace;
*/
	return VK_SUCCESS;
}

VkResult init_Vulkan(VkInstance& vk_instance, VkDevice* out_device)
{
    VkApplicationInfo app_info = {};
    app_info.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext=nullptr;
    app_info.pApplicationName="VT Test";
    app_info.applicationVersion=0x00000001;
    app_info.pEngineName="Noengine";
    app_info.engineVersion=0x00;
    app_info.apiVersion= VK_MAKE_VERSION(1, 0, 2);

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

	VK_THROW(vkCreateInstance(&instance_info, nullptr, &vk_instance));

    VkPhysicalDevice gpu;
    VK_THROW(choose_device(vk_instance, &gpu));

    VK_THROW(create_device(vk_instance, gpu, out_device));

	HINSTANCE instance = GetModuleHandle(NULL);
    HWND window = setup_window(instance, 800, 600, "vk_test");

    VK_THROW(create_surface(vk_instance, gpu, instance, window));

    return VK_SUCCESS;
}

//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
int main(int argc, char *argv[])
{
    VkInstance vk_instance;
    VkDevice device;
    // This is the catch
    if (VkResult err = init_Vulkan(vk_instance, &device))
    {
        print_vk_error_code("Unable to initialize Vulkan: ", err);
        return 1;
    }

	vkDestroyDevice(device, nullptr);
    vkDestroyInstance(vk_instance, nullptr);

    return 0;
}