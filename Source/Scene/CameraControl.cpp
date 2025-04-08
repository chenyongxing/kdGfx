#include "CameraControl.h"

namespace kdGfx
{
	void CameraControl::ResetState()
	{
		for (auto& state : _keyboardStates)
		{
			state = false;
		}
		for (auto& state : _mouseButtonStates)
		{
			state = false;
		}
	}

	void CameraControl::KeyboardControlUpdate(KeyboardControl control)
	{
		_keyboardStates[control] = true;
	}

	void CameraControl::MouseButtonUpdate(MouseButton button)
	{
		_mouseButtonStates[button] = true;
	}

	void CameraControl::MousePositonUpdate(float x, float y)
	{
		_mousePosPrev = _mousePos;
		_mousePos = glm::vec2(x, y);
	}

	void CameraControl::update(float deltaTime)
	{
		if (_camera == nullptr)	return;

		glm::mat4 localToWorld = _camera->getLocalToWorldTransform();
		glm::vec3 forward = localToWorld * glm::vec4(0.f, 0.f, 1.f, 0.f);
		glm::vec3 right = localToWorld * glm::vec4(1.f, 0.f, 0.f, 0.f);
		glm::vec3 up = localToWorld * glm::vec4(0.f, 1.f, 0.f, 0.f);
		glm::vec3 upLocal = glm::vec3(0.f, 1.f, 0.f);
		glm::vec3 position = localToWorld * glm::vec4(0.f, 0.f, 0.f, 1.f);

		glm::mat4 rotation{ 1.0f };
		// mouseMove已经是帧率相关的了
		glm::vec2 mouseMove = _mousePos - _mousePosPrev;
		_mousePosPrev = _mousePos;
		if (_mouseButtonStates[MouseButtonLeft] || _mouseButtonStates[MouseButtonRight])
		{
			float yaw = rotateSpeed * mouseMove.x;
			float pitch = rotateSpeed * mouseMove.y;
			rotation = glm::rotate(rotation, -yaw, upLocal);
			rotation = glm::rotate(rotation, -pitch, right);
		}

		glm::vec3 move{ 0.0f };
		float moveStep = deltaTime * moveSpeed;
		if (_keyboardStates[MoveForward])
		{
			move -= forward * moveStep;
		}
		if (_keyboardStates[MoveBackward])
		{
			move += forward * moveStep;
		}
		if (_keyboardStates[MoveLeft])
		{
			move -= right * moveStep;
		}
		if (_keyboardStates[MoveRight])
		{
			move += right * moveStep;
		}
		if (_keyboardStates[MoveUp])
		{
			move += upLocal * moveStep;
		}
		if (_keyboardStates[MoveDown])
		{
			move -= upLocal * moveStep;
		}

		// 旋转基轴和平移原点
		glm::vec3 xAxis = rotation * glm::vec4(right, 0.f);
		glm::vec3 yAxis = rotation * glm::vec4(up, 0.f);
		glm::vec3 zAxis = normalize(cross(xAxis, yAxis));
		glm::vec3 origin = position + move;
		glm::mat4 transform = glm::mat4(glm::vec4(xAxis, 0.0f), glm::vec4(yAxis, 0.0f),
			glm::vec4(zAxis, 0.0f), glm::vec4(origin, 1.0f));
		_camera->setWorldTransform(transform);
	}
}
