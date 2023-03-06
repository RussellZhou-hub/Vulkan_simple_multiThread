#pragma once
#include "../config.h"
#include "../view/vkUtil/memory.h"

/**
	Holds a vertex buffer for a triangle mesh.
*/
class TriangleMesh {
public:
	TriangleMesh(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice);
	~TriangleMesh();
	Buffer vertexBuffer;
private:
	vk::Device logicalDevice;
};