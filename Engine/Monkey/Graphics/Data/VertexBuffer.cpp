#include "Engine.h"
#include "VertexBuffer.h"

#include "Math/Math.h"
#include "Utils/Crc.h"

#include "Vulkan/VulkanRHI.h"
#include "Vulkan/VulkanResources.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanCommandBuffer.h"

#include <string>
#include <cstring>

VertexBuffer::VertexBuffer()
    : m_VertexCount(0)
    , m_DataSize(0)
    , m_CurrentChannels(0)
	, m_Invalid(true)
	, m_Hash(0)
{
    
}

VertexBuffer::~VertexBuffer()
{
	for (int32 i = 0; i < m_Streams.size(); ++i) {
		delete[] m_Datas[i];
	}
    
    DestroyBuffer();
    
	m_Datas.clear();
	m_Streams.clear();
    m_Channels.clear();
	m_VertexInputStateInfo.Clear();
}

void VertexBuffer::UpdateVertexInputState()
{
	m_VertexInputStateInfo.Clear();

    for (int32 i = 0; i < m_Streams.size(); ++i)
	{
		int32 stride = 0;
		uint32 channelMask = m_Streams[i].channelMask;

		for (int32 j = 0; j < m_Channels.size(); ++j)
		{
			VertexAttribute attribute = m_Channels[j].attribute;
			if ((1 << attribute) & channelMask)
			{
				VertexInputDeclareInfo::AttributeDescription inputAttribute;
				inputAttribute.binding   = i;
				inputAttribute.format    = VEToVkFormat(m_Channels[j].format);
				inputAttribute.offset    = stride;
				inputAttribute.attribute = attribute;
				stride += ElementTypeToSize(m_Channels[j].format);
				m_VertexInputStateInfo.AddAttribute(inputAttribute);
			}
		}

		if (stride > 0)
		{
			VertexInputDeclareInfo::BindingDescription vertexInputBinding;
			vertexInputBinding.binding   = i;
			vertexInputBinding.stride    = stride;
			vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			m_VertexInputStateInfo.AddBinding(vertexInputBinding);
		}
	}
		
    m_VertexInputStateInfo.GenerateHash();
}

void VertexBuffer::AddStream(const VertexStreamInfo& streamInfo, const std::vector<VertexChannelInfo>& channels, uint8* dataPtr)
{
	// 一次性加完，未检测创建好Buffer之后还继续添加的情况
	// 计算出顶点数据的长度
	int32 stride = 0;
	for (int32 i = 0; i < channels.size(); ++i) {
		stride += ElementTypeToSize(channels[i].format);
	}
	
	// 校验数量是否一致
	if (m_VertexCount != 0)
	{
		int32 newVertexCount = streamInfo.size / stride;
		if (m_VertexCount != newVertexCount) {
			MLOGE("Vertex data not match : Size=%d, NewSize=%d", m_VertexCount, newVertexCount);
			return;
		}
	}
	
	// 计算出新的ChannelMask
	uint32 newChannelMask = 0;
	for (int32 i = 0; i < channels.size(); ++i)
	{
		newChannelMask = (1 << channels[i].attribute) | newChannelMask;
		m_Channels.push_back(channels[i]);
	}
    
	m_Invalid     = true;
	m_DataSize   += streamInfo.size;
	m_VertexCount = streamInfo.size / stride;
	m_Datas.push_back(dataPtr);
	m_Streams.push_back(streamInfo);
	m_CurrentChannels = m_CurrentChannels | newChannelMask;
}

void VertexBuffer::DestroyBuffer()
{
	if (m_Invalid) {
		return;
	}
    
	VkDevice device = Engine::Get()->GetDeviceHandle();

	for (int32 i = 0; i < m_Streams.size(); ++i)
	{
		vkDestroyBuffer(device, m_Buffers[i], VULKAN_CPU_ALLOCATOR);
		vkFreeMemory(device, m_Memories[i], VULKAN_CPU_ALLOCATOR);
		m_Memories[i] = VK_NULL_HANDLE;
		m_Buffers[i] = VK_NULL_HANDLE;
	}
    
	m_Hash    = 0;
	m_Invalid = true;
	m_Buffers.clear();
	m_Memories.clear();
}

void VertexBuffer::CreateBuffer()
{
	if (!m_Invalid) {
		return;
	}

	m_Invalid = false;
	m_Hash    = 0;
	for (int32 i = 0; i < m_Streams.size(); ++i) {
		m_Hash = Crc::MemCrc32(m_Datas[i], m_Streams[i].size, m_Hash);
	}

	std::shared_ptr<VulkanRHI> vulkanRHI = Engine::Get()->GetVulkanRHI();
	std::shared_ptr<VulkanDevice> vulkanDevice = vulkanRHI->GetDevice();
	VkDevice device = vulkanDevice->GetInstanceHandle();

	VkMemoryRequirements memReqInfo;
	VkMemoryAllocateInfo memAllocInfo;
	ZeroVulkanStruct(memAllocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);

	VkBufferCreateInfo bufferCreateInfo;
	ZeroVulkanStruct(bufferCreateInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);

	m_Buffers.resize(m_Streams.size());
	m_Memories.resize(m_Streams.size());

	// 创建Host端Buffer以及Memory
	std::vector<VkBuffer> hostBuffers(m_Streams.size());
	std::vector<VkDeviceMemory> hostMemories(m_Streams.size());

	for (int32 i = 0; i < m_Streams.size(); ++i)
	{
		// 创建Buffer
		bufferCreateInfo.size  = m_Streams[i].size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VERIFYVULKANRESULT(vkCreateBuffer(device, &bufferCreateInfo, VULKAN_CPU_ALLOCATOR, &hostBuffers[i]));

		// 获取需要分配的信息
		vkGetBufferMemoryRequirements(device, hostBuffers[i], &memReqInfo);
		uint32 memoryTypeIndex = 0;
		vulkanRHI->GetDevice()->GetMemoryManager().GetMemoryTypeFromProperties(memReqInfo.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryTypeIndex);
		memAllocInfo.allocationSize  = memReqInfo.size;
		memAllocInfo.memoryTypeIndex = memoryTypeIndex;

		// 记录分配信息
		m_Streams[i].alignment = uint32(memReqInfo.alignment);
		m_Streams[i].allocationSize = uint32(memReqInfo.size);
		
		// 创建分配内存并绑定
		VERIFYVULKANRESULT(vkAllocateMemory(device, &memAllocInfo, VULKAN_CPU_ALLOCATOR, &hostMemories[i]));
		VERIFYVULKANRESULT(vkBindBufferMemory(device, hostBuffers[i], hostMemories[i], 0));

		// 映射内存并拷贝数据
		void* dataPtr = nullptr;
		VERIFYVULKANRESULT(vkMapMemory(device, hostMemories[i], 0, memAllocInfo.allocationSize, 0, &dataPtr));
		std::memcpy(dataPtr, m_Datas[i], m_Streams[i].size);
		vkUnmapMemory(device, hostMemories[i]);
	}
	
	// 创建Device端Buffer以及Memory
	for (int32 i = 0; i < m_Streams.size(); ++i)
	{
		// 创建Buffer
		bufferCreateInfo.size  = m_Streams[i].size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		VERIFYVULKANRESULT(vkCreateBuffer(device, &bufferCreateInfo, VULKAN_CPU_ALLOCATOR, &m_Buffers[i]));

		// 获取分配信息
		vkGetBufferMemoryRequirements(device, m_Buffers[i], &memReqInfo);
		uint32 memoryTypeIndex = 0;
		vulkanRHI->GetDevice()->GetMemoryManager().GetMemoryTypeFromProperties(memReqInfo.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex);
		memAllocInfo.allocationSize  = memReqInfo.size;
		memAllocInfo.memoryTypeIndex = memoryTypeIndex;
		
		// 分配内存并绑定
		VERIFYVULKANRESULT(vkAllocateMemory(device, &memAllocInfo, VULKAN_CPU_ALLOCATOR, &m_Memories[i]));
		VERIFYVULKANRESULT(vkBindBufferMemory(device, m_Buffers[i], m_Memories[i], 0));
	}
	
	VulkanCommandListContextImmediate& context = Engine::Get()->GetVulkanDevice()->GetImmediateContext();
	VulkanCmdBuffer* cmdBuffer = context.GetCommandBufferManager()->GetUploadCmdBuffer();

	for (int32 i = 0; i < m_Streams.size(); ++i)
	{
		VkBufferCopy copyRegion = {};
		copyRegion.size = m_Streams[i].allocationSize;
		vkCmdCopyBuffer(cmdBuffer->GetHandle(), hostBuffers[i], m_Buffers[i], 1, &copyRegion);
	}

	context.GetCommandBufferManager()->SubmitUploadCmdBuffer();

	for (int32 i = 0; i < m_Streams.size(); ++i)
	{
		vkDestroyBuffer(device, hostBuffers[i], VULKAN_CPU_ALLOCATOR);
		vkFreeMemory(device, hostMemories[i], VULKAN_CPU_ALLOCATOR);
	}
}
