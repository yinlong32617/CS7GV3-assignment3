#pragma once

#include "maths_funcs.h"

#define PI 3.14159

enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera
{
public:
	vec3 Position;
	vec3 Front;
	vec3 Up;
	vec3 Right;
	vec3 WorldUp;

	float Yaw;
	float Pitch;

	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	Camera(vec3 position = vec3(0, 0, 0), vec3 up = vec3(0, 1, 0), float yaw = YAW, float pitch = PITCH)
		:Front(vec3(0, 0, -1)),
		MovementSpeed(SPEED),
		MouseSensitivity(SENSITIVITY), 
		Zoom(ZOOM)
	{
		this->Position = position;
		this->WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	mat4 GetViewMatrix()
	{
		return look_at(Position, Position + Front, Up);
	}

	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		float velocity = MovementSpeed * deltaTime;
		if (direction == FORWARD)
			Position += Front * velocity;
		if (direction == BACKWARD)
			Position -= Front * velocity;
		if (direction == LEFT)
			Position -= Right * velocity;
		if (direction == RIGHT)
			Position += Right * velocity;
	}

	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		if (constrainPitch)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		}
		updateCameraVectors();
	}

private:
	float radians(float angle)
	{
		return angle * PI / 180;
	}
	void updateCameraVectors()
	{
		vec3 front;
		front.v[0] = cos(radians(Yaw)) * cos(radians(Pitch));
		front.v[1] = sin(radians(Pitch));
		front.v[2] = sin(radians(Yaw)) * cos(radians(Pitch));
		Front = normalise(front);
		this->Right = normalise(cross(Front, WorldUp));
		this->Up = normalise(cross(Right, Front));
	}
};