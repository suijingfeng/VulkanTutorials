#pragma once

#include "Common/Common.h"

#include "VulkanPlatform.h"
#include "VulkanQueue.h"
#include "VulkanMemory.h"
#include "VulkanRHI.h"
#include "VulkanContext.h"

#include <vector>
#include <memory>
#include <map>

class VulkanFenceManager;
class VulkanDeviceMemoryManager;
class VulkanResourceHeapManager;
class VulkanPipelineStateManager;
class VulkanDescriptorPoolsManager;

class VulkanDevice
{
public:
    VulkanDevice(VkPhysicalDevice physicalDevice);
    
    virtual ~VulkanDevice();
    
    bool QueryGPU(int32 deviceIndex);
    
    void InitGPU(int32 deviceIndex);
    
    void CreateDevice();
    
    void Destroy();
    
    bool IsFormatSupported(VkFormat format);
    
    const VkComponentMapping& GetFormatComponentMapping(PixelFormat format) const;
    
    void SetupPresentQueue(VkSurfaceKHR surface);
    
    inline std::shared_ptr<VulkanQueue> GetGraphicsQueue()
    {
        return m_GfxQueue;
    }
    
    inline std::shared_ptr<VulkanQueue> GetComputeQueue()
    {
        return m_ComputeQueue;
    }
    
    inline std::shared_ptr<VulkanQueue> GetTransferQueue()
    {
        return m_TransferQueue;
    }
    
    inline std::shared_ptr<VulkanQueue> GetPresentQueue()
    {
        return m_PresentQueue;
    }
    
    inline VkPhysicalDevice GetPhysicalHandle() const
    {
        return m_PhysicalDevice;
    }
    
    inline const VkPhysicalDeviceProperties& GetDeviceProperties() const
    {
        return m_PhysicalDeviceProperties;
    }
    
    inline const VkPhysicalDeviceLimits& GetLimits() const
    {
        return m_PhysicalDeviceProperties.limits;
    }
    
    inline const VkPhysicalDeviceFeatures& GetPhysicalFeatures() const
    {
        return m_PhysicalDeviceFeatures;
    }
    
    inline VkDevice GetInstanceHandle() const
    {
        return m_Device;
    }
    
    inline const VkFormatProperties* GetFormatProperties() const
    {
        return m_FormatProperties;
    }
    
	inline VulkanFenceManager& GetFenceManager()
	{
		return *m_FenceManager;
	}
    
    inline VulkanDeviceMemoryManager& GetMemoryManager()
    {
        return *m_MemoryManager;
    }
    
    inline VulkanResourceHeapManager& GetResourceHeapManager()
    {
        return *m_ResourceHeapManager;
    }

	inline VulkanPipelineStateManager& GetPipelineStateManager()
	{
		return *m_PipelineStateManager;
	}
    
    inline VulkanDescriptorPoolsManager& GetDescriptorPoolsManager()
    {
        return *m_DescriptorPoolsManager;
    }

	inline VulkanCommandListContextImmediate& GetImmediateContext()
	{
		return *m_ImmediateContext;
	}

private:
    
    void MapFormatSupport(PixelFormat format, VkFormat vkFormat);
    
    void MapFormatSupport(PixelFormat format, VkFormat vkormat, int32 blockBytes);
    
    void SetComponentMapping(PixelFormat format, VkComponentSwizzle r, VkComponentSwizzle g, VkComponentSwizzle b, VkComponentSwizzle a);
    
    void GetDeviceExtensionsAndLayers(std::vector<const char*>& outDeviceExtensions, std::vector<const char*>& outDeviceLayers, bool& bOutDebugMarkers);
    
    void SetupFormats();
    
private:
    friend class VulkanRHI;

private:
    VkDevice                                m_Device;
    VkPhysicalDevice                        m_PhysicalDevice;
    VkPhysicalDeviceProperties              m_PhysicalDeviceProperties;
    VkPhysicalDeviceFeatures                m_PhysicalDeviceFeatures;
    std::vector<VkQueueFamilyProperties>    m_QueueFamilyProps;
    
    VkFormatProperties                      m_FormatProperties[VK_FORMAT_RANGE_SIZE];
    std::map<VkFormat, VkFormatProperties>  m_ExtensionFormatProperties;
	VkComponentMapping                      m_PixelFormatComponentMapping[PF_MAX];

    std::shared_ptr<VulkanQueue>            m_GfxQueue;
    std::shared_ptr<VulkanQueue>            m_ComputeQueue;
    std::shared_ptr<VulkanQueue>            m_TransferQueue;
    std::shared_ptr<VulkanQueue>            m_PresentQueue;

    VulkanFenceManager*                     m_FenceManager;
    VulkanDeviceMemoryManager*              m_MemoryManager;
    VulkanResourceHeapManager*              m_ResourceHeapManager;
	VulkanPipelineStateManager*             m_PipelineStateManager;
    VulkanDescriptorPoolsManager*           m_DescriptorPoolsManager;
    
    VulkanCommandListContextImmediate*      m_ImmediateContext;
    std::vector<VulkanCommandListContext*>  m_CommandContexts;
};
