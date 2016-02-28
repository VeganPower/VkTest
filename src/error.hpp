#pragma once

#include <vulkan/vulkan.h>

#define VK_THROW(x) if( VkResult err = x ) return err

void print_vk_error_code(char const* preable, VkResult err);