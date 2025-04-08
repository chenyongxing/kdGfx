#pragma once

#include "Assets.h"

namespace kdGfx
{
	class Node
	{
	public:
		// 当前节点是模型的根节点
		std::string modelRoot;
		std::string name;
		Node* parent = nullptr;
		std::vector<Node*> children;
		// 本地变换分量
		glm::vec3 translation{ 0.0f };
		glm::vec3 scale{ 1.0f };
		glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };

	public:
		virtual ~Node();

		inline bool IsModelRoot() { return !modelRoot.empty(); };

		inline std::string getPath()
		{
			std::string path(name);
			Node* parent = this->parent;
			while (parent)
			{
				path = parent->name + "/" + path;
				parent = parent->parent;
			}
			return path;
		}

		inline glm::vec3 getWorldPosition()
		{
			glm::mat4 worldTransform = getWorldTransform();
			return glm::vec3(worldTransform[3][0], worldTransform[3][1], worldTransform[3][2]);
		}

		inline void setLocalTransform(const glm::mat4& transform)
		{
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(transform, scale, rotation, translation, skew, perspective);
		}

		inline void setWorldTransform(const glm::mat4& transform)
		{
			if (!parent)	setLocalTransform(transform);
			else	setLocalTransform(parent->getWorldToLocalTransform() * transform);
		}

		inline glm::mat4 getLocalTransform()
		{
			return glm::translate(glm::mat4(1.0f), translation) *
				glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
		}

		inline glm::mat4 getWorldTransform()
		{
			glm::mat4 transform = getLocalTransform();
			Node* parent = this->parent;
			while (parent)
			{
				transform = parent->getLocalTransform() * transform;
				parent = parent->parent;
			}
			return transform;
		}

		inline glm::mat4 getLocalToWorldTransform() { return getWorldTransform(); }

		inline glm::mat4 getWorldToLocalTransform()
		{
			return glm::inverse(getLocalToWorldTransform());
		}
	};

	class Camera final : public Node
	{
	public:
		enum struct Type
		{
			Perspective = 0,
			Orthographic = 1
		};

		Type type = Type::Perspective;
		float fovY = 60.0f;
		float aspect = 1.0f;
		float nearZ = 0.01f;
		float farZ = 100.0f;

		float focalDistance = 1.0f;
		float aperture = 0.0f;

		glm::vec2 jitter{ 0.0f };

	public:
		inline void lookAt(const glm::vec3& eye, const glm::vec3& target)
		{
			glm::mat4 inverseWorldTransform = glm::lookAt(eye, target, glm::vec3(0.f, 1.f, 0.f));
			setWorldTransform(glm::inverse(inverseWorldTransform));
		}

		inline glm::mat4 getProjectionMatrix()
		{
			glm::mat4 projMatrix = glm::perspective(glm::radians(fovY), this->aspect, nearZ, farZ);
			projMatrix[2][0] += jitter.x;
			projMatrix[2][1] += jitter.y;
			return projMatrix;
		}

		inline glm::mat4 getViewMatrix() { return getWorldToLocalTransform(); }

		inline glm::vec2 projectPoint(const glm::vec3& p, const glm::vec4& viewport)
		{
			glm::mat4 view = getViewMatrix();
			glm::mat4 projection = getProjectionMatrix();
			glm::vec4 point = projection * view * glm::vec4(p, 1.f);
			if (point.z < -0.9999f)	return { -FLT_MAX, -FLT_MAX };
			point /= point.w;
			glm::vec2 ndc;
			ndc.x = (point.x + 1.0f) * 0.5f * viewport.z + viewport.x;
			ndc.y = (-point.y + 1.0f) * 0.5f * viewport.w + viewport.y;
			return ndc;
		}
	};

	class Light : public Node
	{
	public:
		size_t index = 0;

		glm::vec3 color{1.0f};
		float intensity = 1.0f;
	};

	class DirectionalLight : public Light
	{
	public:
		float angularDiameter = 0.53f;
	};

	class MeshInstance final : public Node
	{
	public:
		// submesh count = material count
		Mesh* mesh = nullptr;
		std::vector<Material*> materials;
	};
}
