#include "config.hpp"

#include "error.hpp"
#include <iostream>

void print_vk_error_code(char const* preable, VkResult err)
{
    const char* err_string;
    switch(err)
    {
    case VK_SUCCESS: err_string = "VK_SUCCESS"; break;
    case VK_NOT_READY: err_string = "VK_NOT_READY"; break;
    case VK_TIMEOUT: err_string = "VK_TIMEOUT"; break;
    case VK_EVENT_SET: err_string = "VK_EVENT_SET"; break;
    case VK_EVENT_RESET: err_string = "VK_EVENT_RESET"; break;
    case VK_INCOMPLETE: err_string = "VK_INCOMPLETE"; break;
    case VK_ERROR_OUT_OF_HOST_MEMORY: err_string = "VK_ERROR_OUT_OF_HOST_MEMORY"; break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: err_string = "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break;
    case VK_ERROR_INITIALIZATION_FAILED: err_string = "VK_ERROR_INITIALIZATION_FAILED"; break;
    case VK_ERROR_DEVICE_LOST: err_string = "VK_ERROR_DEVICE_LOST"; break;
    case VK_ERROR_MEMORY_MAP_FAILED: err_string = "VK_ERROR_MEMORY_MAP_FAILED"; break;
    case VK_ERROR_LAYER_NOT_PRESENT: err_string = "VK_ERROR_LAYER_NOT_PRESENT"; break;
    case VK_ERROR_EXTENSION_NOT_PRESENT: err_string = "VK_ERROR_EXTENSION_NOT_PRESENT"; break;
    case VK_ERROR_FEATURE_NOT_PRESENT: err_string = "VK_ERROR_FEATURE_NOT_PRESENT"; break;
    case VK_ERROR_INCOMPATIBLE_DRIVER: err_string = "VK_ERROR_INCOMPATIBLE_DRIVER"; break;
    case VK_ERROR_TOO_MANY_OBJECTS: err_string = "VK_ERROR_TOO_MANY_OBJECTS"; break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED: err_string = "VK_ERROR_FORMAT_NOT_SUPPORTED"; break;
    case VK_ERROR_SURFACE_LOST_KHR: err_string = "VK_ERROR_SURFACE_LOST_KHR"; break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: err_string = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"; break;
    case VK_SUBOPTIMAL_KHR: err_string = "VK_SUBOPTIMAL_KHR"; break;
    case VK_ERROR_OUT_OF_DATE_KHR: err_string = "VK_ERROR_OUT_OF_DATE_KHR"; break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: err_string = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"; break;
    case VK_ERROR_VALIDATION_FAILED_EXT: err_string = "VK_ERROR_VALIDATION_FAILED_EXT"; break;
    default: err_string = "Unknown error"; break;
    }

	std::cout << preable << err_string << "\n";
}