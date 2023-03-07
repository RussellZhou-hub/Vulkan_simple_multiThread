#include "engine.h"
#include "vkInit/instance.h"
#include "control/logging.h"
#include"vkInit/device.h"
#include"vkInit/swapchain.h"
#include"vkInit/pipeline.h"
#include"vkInit/framebuffer.h"
#include"vkInit/commands.h"
#include"vkInit/sync.h"
#include"vkInit/descriptors.h"
#include"vkUtil/frame.h"

Engine::Engine(int width, int height, GLFWwindow* window, bool debug) {

	this->width = width;
	this->height = height;
	this->window = window;
	debugMode = debug;

	frameNumber_atomic.store(0);

	for (auto& idxAvailable: frameIndexAvailable){
		idxAvailable.store(true);
	}
	
	if (debugMode) {
		std::cout << "Making a graphics engine\n";
	}

    make_instance();

	make_device();

	make_descriptor_set_layouts();
	make_pipeline();

	finalize_setup();

	make_assets();
}

void Engine::make_instance() {

	instance = vkInit::make_instance(debugMode, "ID Tech 12");
	dldi = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);

	if (vkLogging::Logger::get_logger()->get_debug_mode()) {
		debugMessenger = vkLogging::make_debug_messenger(instance, dldi);
	}
	VkSurfaceKHR c_style_surface;
	if (glfwCreateWindowSurface(instance, window, nullptr, &c_style_surface) != VK_SUCCESS) {
		if (debugMode) {
			std::cout << "Failed to abstract glfw surface for Vulkan\n";
		}
	}
	else if (debugMode) {
		std::cout << "Successfully abstracted glfw surface for Vulkan\n";
	}
	//copy constructor converts to hpp convention
	surface = c_style_surface;
}

void Engine::make_device()
{
	physicalDevice = vkInit::choose_physical_device(instance,debugMode);
	device = vkInit::create_logical_device(physicalDevice, surface, debugMode);
	std::array<vk::Queue,2> queues = vkInit::get_queues(physicalDevice, device, surface, debugMode);
	graphicsQueue = queues[0];
	presentQueue = queues[1];
	make_swapchain();
	frameNumber=0;
	frameNumberTotal.store(0);
}

void Engine::make_swapchain(){
	vkInit::SwapChainBundle bundle = vkInit::create_swapchain(device, physicalDevice, surface, width, height, debugMode);
	swapchain = bundle.swapchain;
	swapchainFrames = bundle.frames;
	swapchainFormat = bundle.format;
	swapchainExtent = bundle.extent;
	maxFramesInFlight=static_cast<int>(swapchainFrames.size());

	std::cout<<"maxFramesInFlight: "<<maxFramesInFlight<<" \n";

	for (vkUtil::SwapChainFrame& frame : swapchainFrames) {
		frame.logicalDevice = device;
		frame.physicalDevice = physicalDevice;
		frame.width = swapchainExtent.width;
		frame.height = swapchainExtent.height;

		frame.make_depth_resources();
	}
}

void Engine::recreate_swapchain(){

	width=0;
	height=0;
	while(width==0 || height==0){
		glfwGetFramebufferSize(window,&width,&height);
		glfwWaitEvents();
	}
	
	device.waitIdle();

	cleanup_swapchain();
	make_swapchain();
	make_framebuffers();
	make_frame_resources();
	vkInit::commandBufferInputChunk commandBufferInput = { device, commandPool, swapchainFrames };
	vkInit::make_frame_command_buffers(commandBufferInput,debugMode);
}

void Engine::make_descriptor_set_layouts(){
	vkInit::descriptorSetLayoutData bindings;
	bindings.count=2;

	bindings.indices.push_back(0);
	bindings.types.push_back(vk::DescriptorType::eUniformBuffer);
	bindings.counts.push_back(1);
	bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

	bindings.indices.push_back(1);
	bindings.types.push_back(vk::DescriptorType::eStorageBuffer);
	bindings.counts.push_back(1);
	bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);

	frameSetLayout = vkInit::make_descriptor_set_layout(device,bindings);

	bindings.count=1;

	bindings.indices[0]=0;
	bindings.types[0]=vk::DescriptorType::eCombinedImageSampler;
	bindings.counts[0]=1;
	bindings.stages[0]=vk::ShaderStageFlagBits::eFragment;

	meshSetLayout = vkInit::make_descriptor_set_layout(device,bindings);
}

void Engine::make_pipeline(){
	vkInit::GraphicsPipelineInBundle specification = {};
	specification.device = device;
	specification.vertexFilepath = "shaders/vertex.spv";
	specification.fragmentFilepath = "shaders/fragment.spv";
	specification.swapchainExtent = swapchainExtent;
	specification.swapchainImageFormat = swapchainFormat;
	specification.depthFormat = swapchainFrames[0].depthFormat;
	specification.descriptorSetLayouts = { frameSetLayout, meshSetLayout };

	vkInit::GraphicsPipelineOutBundle output = vkInit::create_graphics_pipeline(
		specification
	);

	pipelineLayout = output.layout;
	renderpass = output.renderpass;
	pipeline = output.pipeline;
}

void Engine::make_framebuffers(){
	vkInit::framebufferInput frameBufferInput;
	frameBufferInput.device = device;
	frameBufferInput.renderpass = renderpass;
	frameBufferInput.swapchainExtent = swapchainExtent;
	vkInit::make_framebuffers(frameBufferInput, swapchainFrames, debugMode);
}

void Engine::make_frame_resources(){

	vkInit::descriptorSetLayoutData bindings;
	bindings.count=2;
	bindings.indices.push_back(0);
	bindings.indices.push_back(1);
	bindings.types.push_back(vk::DescriptorType::eUniformBuffer);
	bindings.types.push_back(vk::DescriptorType::eStorageBuffer);
	bindings.counts.push_back(1);
	bindings.counts.push_back(1);
	bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);
	bindings.stages.push_back(vk::ShaderStageFlagBits::eVertex);
	frameDescriptorPool = vkInit::make_descriptor_pool(device,static_cast<uint32_t>(swapchainFrames.size()),bindings);

	for(vkUtil::SwapChainFrame& frame: swapchainFrames){
		frame.inFlight = vkInit::make_fence(device, debugMode);
		frame.imageAvailable = vkInit::make_semaphore(device, debugMode);
		frame.renderFinished = vkInit::make_semaphore(device, debugMode);

		frame.make_descriptor_resources();

		frame.descriptorSet = vkInit::allocate_descriptor_set(device,frameDescriptorPool,frameSetLayout);
	}
}

void Engine::finalize_setup(){

	make_framebuffers();

	commandPool = vkInit::make_command_pool(device, physicalDevice, surface, debugMode);

	vkInit::commandBufferInputChunk commandBufferInput = { device, commandPool, swapchainFrames };
	mainCommandBuffer = vkInit::make_command_buffer(commandBufferInput, debugMode);
	vkInit::make_frame_command_buffers(commandBufferInput,debugMode);

	make_frame_resources();
	
}

void Engine::make_assets(){
	meshes = new VertexMenagerie();
	std::vector<float> vertices = { {
		 0.0f, -0.1f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f, //0
		 0.1f, 0.1f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  //1
		-0.1f, 0.1f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  //2
	} };
	std::vector<uint32_t> indices = { {
			0, 1, 2
	} };
	meshTypes type = meshTypes::TRIANGLE;
	meshes->consume(type, vertices, indices);

	vertices = { {
		-0.1f,  0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, //0
		-0.1f, -0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, //1
		 0.1f, -0.1f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, //2
		 0.1f,  0.1f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, //3
	} };
	indices = { {
			0, 1, 2,
			2, 3, 0
	} };
	type = meshTypes::SQUARE;
	meshes->consume(type, vertices, indices);

	vertices = { {
		-0.1f, -0.05f, 1.0f, 1.0f, 1.0f, 0.0f, 0.25f, //0
		-0.04f, -0.05f, 1.0f, 1.0f, 1.0f, 0.3f, 0.25f, //1
		-0.06f,   0.0f, 1.0f, 1.0f, 1.0f, 0.2f,  0.5f, //2
		  0.0f,  -0.1f, 1.0f, 1.0f, 1.0f, 0.5f,  0.0f, //3
		 0.04f, -0.05f, 1.0f, 1.0f, 1.0f, 0.7f, 0.25f, //4
		  0.1f, -0.05f, 1.0f, 1.0f, 1.0f, 1.0f, 0.25f, //5
		 0.06f,   0.0f, 1.0f, 1.0f, 1.0f, 0.8f,  0.5f, //6
		 0.08f,   0.1f, 1.0f, 1.0f, 1.0f, 0.9f,  1.0f, //7
		  0.0f,  0.02f, 1.0f, 1.0f, 1.0f, 0.5f,  0.6f, //8
		-0.08f,   0.1f, 1.0f, 1.0f, 1.0f, 0.1f,  1.0f  //9
	} };
	indices = { {
			0, 1, 2, 
			1, 3, 4, 
			2, 1, 4, 
			4, 5, 6, 
			2, 4, 6, 
			6, 7, 8, 
			2, 6, 8, 
			2, 8, 9  
	} };
	type = meshTypes::STAR;
	meshes->consume(type, vertices, indices);

	FinalizationChunk finalizationChunk;
	finalizationChunk.logicalDevice = device;
	finalizationChunk.physicalDevice=physicalDevice;
	finalizationChunk.queue=graphicsQueue;
	finalizationChunk.commandBuffer=mainCommandBuffer;
	meshes->finalize(finalizationChunk);

	//Materials
	std::unordered_map<meshTypes, const char*> filenames = {
		{meshTypes::TRIANGLE, "tex/face.jpg"},
		{meshTypes::SQUARE, "tex/haus.jpg"},
		{meshTypes::STAR, "tex/noroi.jpg"}
	};

	//Make a descriptor pool to allocate sets.
	vkInit::descriptorSetLayoutData bindings;
	bindings.count = 1;
	bindings.types.push_back(vk::DescriptorType::eCombinedImageSampler);

	meshDescriptorPool = vkInit::make_descriptor_pool(device, static_cast<uint32_t>(filenames.size()), bindings);
	//meshDescriptorPool = vkInit::make_descriptor_pool(device, 3, bindings);

	vkImage::TextureInputChunk textureInfo;
	textureInfo.commandBuffer = mainCommandBuffer;
	textureInfo.queue = graphicsQueue;
	textureInfo.logicalDevice = device;
	textureInfo.physicalDevice = physicalDevice;
	textureInfo.layout = meshSetLayout;
	textureInfo.descriptorPool = meshDescriptorPool;

	for (const auto & [object, filename] : filenames) {
		textureInfo.filename = filename;
		materials[object] = new vkImage::Texture(textureInfo);
	}
}

void Engine::prepare_frame(uint32_t imageIndex, Scene* scene){

	vkUtil::SwapChainFrame _frame = swapchainFrames[imageIndex];

	glm::vec3 eye = {1.0f,0.0f,-1.0f};
	glm::vec3 center = {0.0f,0.0f,0.0f};
	glm::vec3 up = {0.0f,0.0f,-1.0f};
	glm::mat4 view = glm::lookAt(eye,center,up);

	glm::mat4 projection = glm::perspective(
		glm::radians(45.0f),
		static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height),
		0.1f,10.0f);

	projection[1][1] *= -1;

	_frame.cameraData.view = view;
	_frame.cameraData.projection = projection;
	_frame.cameraData.viewProjection = projection * view;
	memcpy(_frame.cameraDataWriteLocation, &(_frame.cameraData), sizeof(vkUtil::UBO));

	size_t i = 0;
	for(glm::vec3& position:scene->trianglesPositions){
		_frame.modelTransforms[i++] = glm::translate(glm::mat4(1.0f),position);
	}
	for(glm::vec3& position:scene->squarePositions){
		_frame.modelTransforms[i++] = glm::translate(glm::mat4(1.0f),position);
	}
	for(glm::vec3& position:scene->starPositions){
		_frame.modelTransforms[i++] = glm::translate(glm::mat4(1.0f),position);
	}
	memcpy(_frame.modelBufferWriteLocation, _frame.modelTransforms.data(), i*sizeof(glm::mat4));

	swapchainFrames[imageIndex].write_descriptor_set();
}

void Engine::prepare_scene(vk::CommandBuffer commandBuffer){
	vk::Buffer vertexBuffers[] = { meshes->vertexBuffer.buffer };
	vk::DeviceSize offsets[] = {0};
	commandBuffer.bindVertexBuffers(0,1,vertexBuffers,offsets);
	commandBuffer.bindIndexBuffer(meshes->indexBuffer.buffer, 0, vk::IndexType::eUint32);
}

void Engine::record_draw_commands(vk::CommandBuffer commandBuffer, uint32_t imageIndex,Scene* scene){
	vk::CommandBufferBeginInfo beginInfo = {};

	try {
		commandBuffer.begin(beginInfo);
	}
	catch (vk::SystemError err) {
		if (debugMode) {
			std::cout << "Failed to begin recording command buffer!" << std::endl;
		}
	}

	vk::RenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.renderPass = renderpass;
	renderPassInfo.framebuffer = swapchainFrames[imageIndex].framebuffer;
	renderPassInfo.renderArea.offset.x = 0;
	renderPassInfo.renderArea.offset.y = 0;
	renderPassInfo.renderArea.extent = swapchainExtent;

	vk::ClearValue colorClear;
	std::array<float, 4> colors = { 1.0f, 0.5f, 0.25f, 1.0f };
	colorClear.color = vk::ClearColorValue(colors);
	vk::ClearValue depthClear;

#ifdef VK_MAKE_VERSION
	depthClear.depthStencil = vk::ClearDepthStencilValue({ 1.0f },{0});
#else
	depthClear.depthStencil = vk::ClearDepthStencilValue({ 1.0f,0 });
#endif
	
	std::vector<vk::ClearValue> clearValues = { {colorClear, depthClear} };

	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();

	commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

	commandBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		pipelineLayout,0,swapchainFrames[imageIndex].descriptorSet,nullptr);


	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

	prepare_scene(commandBuffer);

	uint32_t startInstance = 0;
	//Triangles
	render_objects(
		commandBuffer, meshTypes::TRIANGLE, startInstance, static_cast<uint32_t>(scene->trianglesPositions.size())
	);

	//Squares
	render_objects(
		commandBuffer, meshTypes::SQUARE, startInstance, static_cast<uint32_t>(scene->squarePositions.size())
	);

	//Stars
	render_objects(
		commandBuffer, meshTypes::STAR, startInstance, static_cast<uint32_t>(scene->starPositions.size())
	);
	

	commandBuffer.endRenderPass();

	try {
		commandBuffer.end();
	}
	catch (vk::SystemError err) {
		
		if (debugMode) {
			std::cout << "failed to record command buffer!" << std::endl;
		}
	}
}

void Engine::render_objects(vk::CommandBuffer commandBuffer, meshTypes objectType, uint32_t& startInstance, uint32_t instanceCount) {

	int indexCount = meshes->indexCounts.find(objectType)->second;
	int firstIndex = meshes->firstIndices.find(objectType)->second;
	materials[objectType]->use(commandBuffer, pipelineLayout);
	commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, 0, startInstance);
	startInstance += instanceCount;
}

int Engine::get_maxFramesInFlight(){
	return maxFramesInFlight;
}


void Engine::render(Scene* scene){
	int frameIndex=frameNumber_atomic.load();
	int frameAccumulate=frameNumberTotal.load();

	device.waitForFences(1, &swapchainFrames[frameIndex].inFlight, VK_TRUE, UINT64_MAX);
	
	
	//acquireNextImageKHR(vk::SwapChainKHR, timeout, semaphore_to_signal, fence)
	uint32_t imageIndex;
	//vk::ResultValue<uint32_t> acquire;
	try{
		vk::ResultValue acquire = device.acquireNextImageKHR(swapchain, UINT64_MAX, swapchainFrames[frameIndex].imageAvailable, nullptr);
		imageIndex=acquire.value;
	} catch(vk::OutOfDateKHRError){
		recreate_swapchain();
		return;
	}
	

	vk::CommandBuffer commandBuffer = swapchainFrames[imageIndex].commandBuffer;

	
#ifdef VK_MAKE_VERSION
	VULKAN_HPP_NAMESPACE::CommandBufferResetFlags flags={};
	VULKAN_HPP_NAMESPACE::DispatchLoaderStatic d VULKAN_HPP_DEFAULT_DISPATCHER_ASSIGNMENT ;
	commandBuffer.reset(flags,d);
#else 
	commandBuffer.reset();
#endif

	prepare_frame(imageIndex,scene);

	record_draw_commands(commandBuffer, imageIndex,scene);

	vk::SubmitInfo submitInfo = {};

	vk::Semaphore waitSemaphores[] = { swapchainFrames[frameIndex].imageAvailable };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vk::Semaphore signalSemaphores[] = { swapchainFrames[frameIndex].renderFinished };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	device.resetFences(1, &swapchainFrames[frameIndex].inFlight);
	try {
		graphicsQueue.submit(submitInfo, swapchainFrames[frameIndex].inFlight); 
	}
	catch (vk::SystemError err) {
		
		if (debugMode) {
			std::cout << "failed to submit draw command buffer!" << std::endl;
		}
	}

	vk::PresentInfoKHR presentInfo = {};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	vk::SwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vk::Result present;
	try{
		present=presentQueue.presentKHR(presentInfo);
	}
	catch(vk::OutOfDateKHRError error){
		present = vk::Result::eErrorOutOfDateKHR;
	}

	if(present==vk::Result::eErrorOutOfDateKHR || present==vk::Result::eSuboptimalKHR){
		std::cout<<"Recreate" << std::endl;
		recreate_swapchain();
		return;
	}

	frameNumber=(frameNumber+1) % maxFramesInFlight;
	frameNumber_atomic.store((frameIndex+1)% maxFramesInFlight);
	frameNumberTotal.store(frameAccumulate+1);

	//device.waitIdle();
}

void Engine::renderThreadFunc(Scene* scene,int frameIndex){

	while(!shouldClose){

		int frameAccumulate=frameNumberTotal.load();

	
		bool expected=true;
		bool desired=false;
		while(!frameIndexAvailable[frameIndex].compare_exchange_strong(expected,desired)){   // see if other thread is using this index
			frameIndex=(frameIndex+1)%maxFramesInFlight;
		}

		//int frameIndex=frameNumber_atomic.load();
		device.waitForFences(1, &swapchainFrames[frameIndex].inFlight, VK_TRUE, UINT64_MAX);

		/*
		//now we can use this fence resource , but we need to see if other thread is using this
		while(!frameNumber_atomic.compare_exchange_weak(frameIndex,frameIndex)){ // if frameIndex not changed then we continue
			device.resetFences(1, &swapchainFrames[frameIndex].inFlight);   //make this resource outher thread available
			frameIndex = frameNumber_atomic.load(); //reload
			device.waitForFences(1, &swapchainFrames[frameIndex].inFlight, VK_TRUE, UINT64_MAX);
		};
	*/
	
		//acquireNextImageKHR(vk::SwapChainKHR, timeout, semaphore_to_signal, fence)
		uint32_t imageIndex;
		//vk::ResultValue<uint32_t> acquire;
		try{
			vk::ResultValue acquire = device.acquireNextImageKHR(swapchain, UINT64_MAX, swapchainFrames[frameIndex].imageAvailable, nullptr);
			imageIndex=acquire.value;
		} catch(vk::OutOfDateKHRError){
			recreate_swapchain();
			return;
		}
	

		vk::CommandBuffer commandBuffer = swapchainFrames[imageIndex].commandBuffer;

	
#ifdef VK_MAKE_VERSION
		VULKAN_HPP_NAMESPACE::CommandBufferResetFlags flags={};
		VULKAN_HPP_NAMESPACE::DispatchLoaderStatic d VULKAN_HPP_DEFAULT_DISPATCHER_ASSIGNMENT ;
		commandBuffer.reset(flags,d);
#else 
		commandBuffer.reset();
#endif

		prepare_frame(imageIndex,scene);

		record_draw_commands(commandBuffer, imageIndex,scene);

		vk::SubmitInfo submitInfo = {};

		vk::Semaphore waitSemaphores[] = { swapchainFrames[frameIndex].imageAvailable };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vk::Semaphore signalSemaphores[] = { swapchainFrames[frameIndex].renderFinished };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		device.resetFences(1, &swapchainFrames[frameIndex].inFlight);

		try {
			graphicsQueue.submit(submitInfo, swapchainFrames[frameIndex].inFlight); 
		}
		catch (vk::SystemError err) {
		
			if (debugMode) {
				std::cout << "failed to submit draw command buffer!" << std::endl;
			}
		}

		expected=false;
		desired=true;
		if(!frameIndexAvailable[frameIndex].compare_exchange_strong(expected,desired)){   // make this resource available for other thread
			std::cout<<"there is an error: frameIndex"<<frameIndex<<" is be changed unexpected\n";
		}

		vk::PresentInfoKHR presentInfo = {};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		vk::SwapchainKHR swapChains[] = { swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		vk::Result present;
		try{
			present=presentQueue.presentKHR(presentInfo);
		}
		catch(vk::OutOfDateKHRError error){
			present = vk::Result::eErrorOutOfDateKHR;
		}

		if(present==vk::Result::eErrorOutOfDateKHR || present==vk::Result::eSuboptimalKHR){
			std::cout<<"Recreate" << std::endl;
			recreate_swapchain();
			return;
		}

		//frameIndex=(frameIndex+1) % maxFramesInFlight;
		frameNumber_atomic.store((frameIndex+1) % maxFramesInFlight);
		frameNumberTotal.store(frameAccumulate+1);
		}
}

int Engine::getLastTime(){
	return frameTime_atomic.load();
}
	
void Engine::setLatTime(int currentTime){
	frameTime_atomic.store(currentTime);
}

int Engine::getCurrentFrame(){
	return frameNumber_atomic.load();
}

void Engine::cleanup_swapchain(){
	for (vkUtil::SwapChainFrame& frame : swapchainFrames) {
		frame.destroy();
	}

	device.destroySwapchainKHR(swapchain);

	device.destroyDescriptorPool(frameDescriptorPool);
}

Engine::~Engine() {

	device.waitIdle();
	
	if (debugMode) {
		std::cout << "Goodbye see you!\n";
	}

	device.destroyCommandPool(commandPool);

	device.destroyPipeline(pipeline);
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderpass);

	cleanup_swapchain();

	device.destroyDescriptorSetLayout(frameSetLayout);

	delete meshes;

	for (const auto& [key, texture] : materials) {
		delete texture;
	}

	device.destroyDescriptorSetLayout(meshSetLayout);
	device.destroyDescriptorPool(meshDescriptorPool);

	device.destroy();
	instance.destroySurfaceKHR(surface);

	if (vkLogging::Logger::get_logger()->get_debug_mode()) {
		instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, dldi);
	}

    instance.destroy();

	//terminate glfw
	glfwTerminate();
}