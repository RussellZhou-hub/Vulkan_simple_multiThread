#include <vec4.hpp>
#include <mat4x4.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include<vector>
#include<deque>
#include<unordered_map>
#include<string>
#include<set>
#include"Camera.h"

double scrollOffset=0;
bool isScrollChanged = false;
bool isMouseLeftBtnPressed = false;
bool isMouseMiddleBtnPressed = false;

float Camera::getDeltaTime()
{
	return deltaTime;
}

void Camera::setLastTime(float currentTime){
	if (lastFrame == -1.0f){
		deltaTime =0.01f;
	}
	deltaTime = currentTime - lastFrame;
	lastFrame = currentTime;
}