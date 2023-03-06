#pragma once
#include "../config.h"

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
#include "../view/vkMesh/mesh.h"
#include "tiny_obj_loader.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

namespace vkMesh{
    class Vertex {
	public:
	    glm::vec3 pos;
	    glm::vec3 normal;
	    glm::vec2 texCoord;

	    static vk::VertexInputBindingDescription getBindingDescription();

	    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();

	    bool operator==(const Vertex& other) const;
    };

    class Model{
        Model(const std::string& filename);
        void loadModel(const std::string& filename);
        std::vector<char> model_buffer; //read from file
        static std::vector<char> readFile(const std::string& filename);
    };
}

namespace std {
	template<> struct hash<vkMesh::Vertex> {
		size_t operator()(vkMesh::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}