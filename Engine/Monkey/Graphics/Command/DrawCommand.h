﻿#pragma once

#include "Common/Common.h"
#include "HAL/ThreadSafeCounter.h"
#include "Utils/Crc.h"

#include "Vulkan/VulkanPlatform.h"
#include "Vulkan/VulkanResources.h"

#include <memory>
#include <vector>

class VulkanCommandListContext;
class VulkanCmdBuffer;
class Material;
class Renderable;

class MeshDrawCommand
{
public:
	MeshDrawCommand();

	virtual ~MeshDrawCommand();

	void GenerateHash();

	void Prepare(VulkanCmdBuffer* cmdBuffer, VulkanCommandListContext* cmdListContext);

	void Reset();

public:

	std::shared_ptr<Material>			material;
	std::shared_ptr<Renderable>			renderable;

	uint32		firstIndex;
	uint32		numPrimitives;
	uint32		numInstances;

	uint32		hash;
	bool        ready;
};

class MeshDrawListContext
{
public:
	MeshDrawListContext();

	virtual ~MeshDrawListContext();

	virtual MeshDrawCommand* AddCommand(MeshDrawCommand* command);
	
protected:

	std::vector<MeshDrawCommand*> m_DrawCommandList;
};