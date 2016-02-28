CXX=gcc

#debug
# Sample
# CXXFLAGS=-g -Wall -I..\Vulkan\Samples\external -std=c++11 -DVK_PROTOTYPES  -DVK_USE_PLATFORM_WIN32_KHR -DNOMINMAX -m64
#LDFLAGS=-L../Vulkan/Samples/libs/vulkan -lvulkan-1
# LunarG SDK
CXXFLAGS=-g -Wall -IC:\VulkanSDK\1.0.3.1\Include -std=c++11 -DVK_PROTOTYPES  -DVK_USE_PLATFORM_WIN32_KHR -DNOMINMAX -m64 -MMD
# -m64
LDFLAGS=-LC:\VulkanSDK\1.0.3.1\Bin -lvulkan-1  -MP
# -MF

obj_list = main.o error.o
target = vk_test

#.cpp.o:
 #   $(CXX) $(CXXFLAGS) $< -o $@

vk_test: $(obj_list)
	$(CXX) $(LDFLAGS) $(obj_list) -o $(@)

all:vk_test

clean:
	del *.o