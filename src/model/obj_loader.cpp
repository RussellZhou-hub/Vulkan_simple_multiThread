#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/hash.hpp>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <array>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include "../view/vkImage/image.h"
#include "obj_loader.h"


vk::VertexInputBindingDescription vkMesh::Vertex::getBindingDescription() {
		vk::VertexInputBindingDescription bindingDesc{};

		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof(Vertex);
		bindingDesc.inputRate = vk::VertexInputRate::eVertex;

		return bindingDesc;
	}

std::vector<vk::VertexInputAttributeDescription> vkMesh::Vertex::getAttributeDescriptions() {
		std::vector<vk::VertexInputAttributeDescription> attributeDesc{};
		vk::VertexInputAttributeDescription dummy;
		attributeDesc.push_back(dummy);
		attributeDesc.push_back(dummy);
		attributeDesc.push_back(dummy);

		//position
		attributeDesc[0].binding = 0;
		attributeDesc[0].location = 0;
		attributeDesc[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDesc[0].offset = offsetof(Vertex, pos);

		//normal
		attributeDesc[1].binding = 0;
		attributeDesc[1].location = 1;
		attributeDesc[1].format = vk::Format::eR32G32B32Sfloat;
		attributeDesc[1].offset = offsetof(Vertex, normal);

		//texCoord
		attributeDesc[2].binding = 0;
		attributeDesc[2].location = 2;
		attributeDesc[2].format = vk::Format::eR32G32Sfloat;
		attributeDesc[2].offset = offsetof(Vertex, texCoord);

		return attributeDesc;
}

bool vkMesh::Vertex::operator==(const Vertex& other) const {
	return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
}

vkMesh::Model::Model(const std::string& filename){
	model_buffer = readFile(filename);
}

void vkMesh::Model::loadModel(const std::string& filename){
	/*
	tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warning, error;

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, filename.c_str() ) ) {  //"models/statue.obj"
			throw std::runtime_error(warning + error);
		}

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attributes.vertices[3 * index.vertex_index],
					attributes.vertices[3 * index.vertex_index + 1],
					attributes.vertices[3 * index.vertex_index + 2]
				};

				vertex.normal = {
					attributes.normals[3 * index.normal_index],
					attributes.normals[3 * index.normal_index + 1],
					attributes.normals[3 * index.normal_index + 2]
				};

				vertex.texCoord = {
					attributes.texcoords[2 * index.texcoord_index],
					attributes.texcoords[2 * index.texcoord_index + 1]
				};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				indices.push_back(uniqueVertices[vertex]);
			}
		}
	*/
}

std::vector<char> vkMesh::Model::readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	/*
	* ios: input/output stream
	* ate: open starting at the end
	* binary: read binary data
	*/
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file\n");
	}

	size_t fileSize = (size_t)file.tellg(); //filesize = position of read-head at end of file
	std::vector<char> buffer(fileSize);
	file.seekg(0); //rewind to start of file to begin reading
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}