#include "app.h"

App::App(int width, int height, bool debug) {

	build_glfw_window(width, height, debug);

	graphicsEngine = new Engine(width, height, window, debug);

	scene = new Scene();
}

void App::build_glfw_window(int width, int height, bool debugMode) {

	//initialize glfw
	glfwInit();

	//no default rendering client, we'll hook vulkan up
	//to the window later
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//resizing breaks the swapchain, we'll disable it for now
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	//GLFWwindow* glfwCreateWindow (int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
	if (window = glfwCreateWindow(width, height, "ID Tech 12", nullptr, nullptr)) {
		if (debugMode) {
			std::cout << "Successfully made a glfw window called \"ID Tech 12\", width: " << width << ", height: " << height << '\n';
		}
	}
	else {
		if (debugMode) {
			std::cout << "GLFW window creation failed\n";
		}
	}
}

void App::mainLoop(){

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		{
			//setIcon();
			
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

			glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_CURSOR_DISABLED);
            //glfwSetCursorPosCallback(window, (GLFWcursorposfun)mouse_callback);
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            mouse_callback(window, xpos, ypos);
            glfwSetScrollCallback(window, scroll_callback);
            glfwSetMouseButtonCallback(window, mouse_button_callback);
            scroll_process();

            const float cameraSpeed = 100.0f * camera.getDeltaTime(); // adjust accordingly
            camera.setLastTime(glfwGetTime());
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                camera.pos += cameraSpeed * camera.front;
                ubo.frameCount = 1; //camera moved
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                camera.pos -= cameraSpeed * camera.front;
                ubo.frameCount = 1; //camera moved
            }

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                camera.pos -= glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
                ubo.frameCount = 1; //camera moved
            }

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                camera.pos += glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
                ubo.frameCount = 1; //camera moved
            }
        
		}
		//drawFrame();
        graphicsEngine->shouldClose = glfwWindowShouldClose(window);

        graphicsEngine->render(scene);

		//calculateFrameRate();
        calculateFrameRate_multi();
	}
}

void App::mainLoop_multi(){

    
    int numTreads = graphicsEngine->get_maxFramesInFlight()/2;
    std::vector<std::thread> threads;

    Engine& obj = (*graphicsEngine);
    for(auto i=0;i<numTreads;++i){ 
        threads.emplace_back(std::thread([&obj,this,i](){ obj.renderThreadFunc(scene,i); }));
    }
    for(auto& t: threads){ 
        t.join();
    }
    
    // wait rendering threads to finished
    //for(auto& thread:threads){
    //    thread.detach();
    //}

    lastFrameAccumulate=graphicsEngine->frameNumberTotal.load();
    
    lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {

        
        if(lastFrameAccumulate!=0 && lastFrameAccumulate==graphicsEngine->frameNumberTotal.load()){ // if not rendered new frame, skip
            continue;
        }
        lastFrameAccumulate=graphicsEngine->frameNumberTotal.load(); // only used in main thread to see if rendered new image
        

		glfwPollEvents();
		
		{
			//setIcon();
			
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

			glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_CURSOR_DISABLED);
            //glfwSetCursorPosCallback(window, (GLFWcursorposfun)mouse_callback);
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            mouse_callback(window, xpos, ypos);
            glfwSetScrollCallback(window, scroll_callback);
            glfwSetMouseButtonCallback(window, mouse_button_callback);
            scroll_process();

            const float cameraSpeed = 100.0f * camera.getDeltaTime(); // adjust accordingly
            camera.setLastTime(glfwGetTime());
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                camera.pos += cameraSpeed * camera.front;
                ubo.frameCount = 1; //camera moved
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                camera.pos -= cameraSpeed * camera.front;
                ubo.frameCount = 1; //camera moved
            }

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                camera.pos -= glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
                ubo.frameCount = 1; //camera moved
            }

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                camera.pos += glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
                ubo.frameCount = 1; //camera moved
            }
        
		}
		//drawFrame();
        graphicsEngine->shouldClose = glfwWindowShouldClose(window);

        //graphicsEngine->render(scene);

		//calculateFrameRate();
        calculateFrameRate_multi();
	}
}

void App::run() {
	mainLoop();
    //mainLoop_multi();
}

int App::getDeltaTime(Engine* graphicsEngine,int currentTime){

}

void App::calculateFrameRate() {
	currentTime = glfwGetTime();
	double delta = currentTime - lastTime;

	if (delta >= 1) {
		int framerate{ std::max(1, int(numFrames / delta)) };
		std::stringstream title;
		title << "Running at " << framerate << " fps.";
		glfwSetWindowTitle(window, title.str().c_str());
		lastTime = currentTime;
		numFrames = -1;
		frameTime = float(1000.0 / framerate);
	}

	++numFrames;
}

void App::calculateFrameRate_multi() {
	currentTime = glfwGetTime();
	double delta = currentTime - lastTime;
    int currenFrame=graphicsEngine->getCurrentFrame();
    numFrames = graphicsEngine->frameNumberTotal.load();

	if (delta >= 1) {
		int framerate{ std::max(1, int(numFrames / delta)) };
		std::stringstream title;
		title << "Running at " << framerate << " fps.";
		glfwSetWindowTitle(window, title.str().c_str());
		lastTime = currentTime;
		numFrames = -1;
		frameTime = float(1000.0 / framerate);

        lastFrameAccumulate=currenFrame;
        graphicsEngine->frameNumberTotal.store(0);
	}
}

void App::mouse_callback(GLFWwindow* window,double xpos, double ypos){
	if (mouse.isFirstMouse)
    {
        mouse.lastX = xpos;
        mouse.lastY = ypos;
        mouse.isFirstMouse = false;
    }
    if (isMouseLeftBtnPressed) {
        float xoffset = xpos - mouse.lastX;
        float yoffset = mouse.lastY - ypos;
        mouse.lastX = xpos;
        mouse.lastY = ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        mouse.yaw += xoffset;
        mouse.pitch += yoffset;

        if (mouse.pitch > 89.0f)
            mouse.pitch = 89.0f;
        if (mouse.pitch < -89.0f)
            mouse.pitch = -89.0f;

        if (xoffset || yoffset) {
            glm::vec3 direction;
            direction.x = cos(glm::radians(mouse.yaw)) * cos(glm::radians(mouse.pitch));
            direction.y = sin(glm::radians(mouse.pitch));
            direction.z = sin(glm::radians(mouse.yaw)) * cos(glm::radians(mouse.pitch));
            camera.front = glm::normalize(direction);
        }
        ubo.frameCount = 1; //camera moved
    }
    else if (!isMouseLeftBtnPressed&& isMouseMiddleBtnPressed) {
        float xoffset = xpos - mouse.lastX;
        float yoffset = ypos- mouse.lastY;
        mouse.lastX = xpos;
        mouse.lastY = ypos;
        const float cameraSpeed = 1.0f * camera.getDeltaTime(); // adjust accordingly
        camera.pos += cameraSpeed * camera.up* yoffset;
        camera.pos -= glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed*xoffset;
        ubo.frameCount = 1; //camera moved
        //mouse.isFirstMouse = true;
    }
    else {
        mouse.isFirstMouse = true;
    }
}

void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    scrollOffset = yoffset;
    isScrollChanged = true;
}

void App::scroll_process()
{
    if (isScrollChanged) {
        const float cameraSpeed = 100.0f * camera.getDeltaTime(); // adjust accordingly
        camera.fov -= (float)scrollOffset * cameraSpeed;
        if (camera.fov < 1.0f)
            camera.fov = 1.0f;
        if (camera.fov > 45.0f)
            camera.fov = 45.0f;
        isScrollChanged = false;
        ubo.frameCount = 1; //camera moved
    }
}

void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        isMouseLeftBtnPressed=true;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        isMouseLeftBtnPressed = false;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
        isMouseMiddleBtnPressed = true;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
        isMouseMiddleBtnPressed = false;
}

App::~App() {
	delete graphicsEngine;
	delete scene;
}