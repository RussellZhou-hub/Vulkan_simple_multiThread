#pragma once
#include "../config.h"
#include "../view/engine.h"
#include "../model/scene.h"
#include "../model/Camera.h"
#include "../view/vkMesh/mesh.h"

class App {

public:
	double lastTime, currentTime;
	int numFrames;
	int lastFrameAccumulate;
	float frameTime;

private:
	Engine* graphicsEngine;
	GLFWwindow* window;
	Scene* scene;;

	Camera camera;
    Mouse mouse;
    UniformBufferObject ubo;

	void build_glfw_window(int width, int height, bool debugMode);

	void mainLoop();
	void mainLoop_multi();
	
	int getDeltaTime(Engine* graphicsEngine,int currentTime);

	void calculateFrameRate();
	void calculateFrameRate_multi();
	void mouse_callback(GLFWwindow* window,double xpos, double ypos);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void scroll_process();

public:
	App(int width, int height, bool debug);
	~App();
	void run();
};