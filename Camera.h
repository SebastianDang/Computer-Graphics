#pragma once

#include "Window.h"
#include "Definitions.h"
#include "OBJObject.h"

#ifndef CAMERA_H
#define CAMERA_H
class Camera {
private:
	Global_Camera camera;

	void setupCamera(glm::vec3 e, glm::vec3 d, glm::vec3 up);
	void updateCamera();

public:
	Camera(glm::vec3 e, glm::vec3 d, glm::vec3 up);
	~Camera();

	glm::vec3 get_cam_pos();
	glm::vec3 get_cam_look_at();
	glm::vec3 get_cam_up();

	glm::vec3 set_cam_pos(glm::vec3 update);
	glm::vec3 set_cam_look_at(glm::vec3 update);
	glm::vec3 set_cam_up(glm::vec3 update);

	void camera_rotate_around(glm::vec3 v, glm::vec3 w);
	void camera_zoom(double y);

	void window_updateCamera();


};
#endif