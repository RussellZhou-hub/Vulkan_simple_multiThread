#pragma once
#include "../config.h"
#include"vkUtil/frame.h"
#include "../model/scene.h"
#include "../model/triangle_mesh.h"
#include "../model/vertex_menagerie.h"
#include "vkImage/image.h"

class Engine {

public:

	Engine(int width, int height, GLFWwindow* window, bool debug);

	~Engine();

	void render(Scene* scene);
	void renderThreadFunc(Scene* scene,int frameIndex);
	int get_maxFramesInFlight();
	int getLastTime();
	void setLatTime(int currentTime);
	int getCurrentFrame();

	bool shouldClose;
	std::atomic<int> frameNumberTotal;

	static const int kBufferSize=10;
	std::atomic<bool> frameIndexAvailable[kBufferSize];

private:

	//whether to print debug messages in functions
	bool debugMode;

	//glfw window parameters
	int width{ 640 };
	int height{ 480 };
	GLFWwindow* window{ nullptr };

    //instance-related variables
    vk::Instance instance{nullptr};
	vk::DebugUtilsMessengerEXT debugMessenger{ nullptr };
	vk::DispatchLoaderDynamic dldi;
	vk::SurfaceKHR surface;

	//device-related variable
	vk::PhysicalDevice physicalDevice{nullptr};
	vk::Device device{ nullptr };
	vk::Queue graphicsQueue{ nullptr };
	vk::Queue presentQueue{ nullptr };
	vk::SwapchainKHR swapchain{ nullptr };
	std::vector<vkUtil::SwapChainFrame> swapchainFrames;
	vk::Format swapchainFormat;
	vk::Extent2D swapchainExtent;

	//pipeline-related variables
	vk::PipelineLayout pipelineLayout;
	vk::RenderPass renderpass;
	vk::Pipeline pipeline;

	//Command-related variables
	vk::CommandPool commandPool;
	vk::CommandBuffer mainCommandBuffer;

	//Synchronization objects
	int maxFramesInFlight,frameNumber;
	std::atomic<int> frameNumber_atomic; //for multiThread rendering
	std::atomic<int> frameTime_atomic; //for multiThread rendering

	//descriptor-related variables
	vk::DescriptorSetLayout frameSetLayout;
	vk::DescriptorPool frameDescriptorPool; //Descriptors bound on a "per frame" basis
	vk::DescriptorSetLayout meshSetLayout;
	vk::DescriptorPool meshDescriptorPool; //Descriptors bound on a "per mesh" basis

	//asset pointers
	VertexMenagerie* meshes;
	std::unordered_map<meshTypes,vkImage::Texture*> materials;

    //instance setup
	void make_instance();

	//device setup
	void make_device();
	void make_swapchain();
	void recreate_swapchain();

	//pipline setup
	void make_descriptor_set_layouts();
	void make_pipeline();

	//final setup steps
	void finalize_setup();
	void make_framebuffers();
	void make_frame_resources();

	void make_assets();
	void prepare_scene(vk::CommandBuffer);
	void prepare_frame(uint32_t imageIndex, Scene* scene);

	void record_draw_commands(vk::CommandBuffer commandBuffer, uint32_t imageIndex, Scene* scene);
	void render_objects(vk::CommandBuffer commandBuffer, meshTypes objectType, uint32_t& startInstance, uint32_t instanceCount);

	void cleanup_swapchain();
};