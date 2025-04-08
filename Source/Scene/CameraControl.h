#pragma once

#include "Node.h"

namespace kdGfx
{
	class CameraControl final
	{
	public:
		float moveSpeed = 10.0f;
		float rotateSpeed = 0.002f;

	public:
		enum KeyboardControl
		{
			MoveForward,
			MoveBackward,
			MoveLeft,
			MoveRight,
			MoveUp,
			MoveDown,
			KeyboardControlCount
		};

		enum MouseButton
		{
			MouseButtonLeft,
			MouseButtonMiddle,
			MouseButtonRight,
			MouseButtonCount
		};

		void ResetState();
		void KeyboardControlUpdate(KeyboardControl control);
		void MouseButtonUpdate(MouseButton button);
		void MousePositonUpdate(float x, float y);
		void update(float deltaTime);

		inline void setCamera(Camera* camera) { _camera = camera; }

	private:
		Camera* _camera = nullptr;
		glm::vec2 _mousePos{ 0.0f };
		glm::vec2 _mousePosPrev{ 0.0f };
		std::array<bool, KeyboardControlCount> _keyboardStates = { false };
		std::array<bool, MouseButtonCount> _mouseButtonStates = { false };
	};
}
