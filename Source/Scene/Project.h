#pragma once

#include "Scene.h"
#include "Misc.h"

namespace kdGfx
{
	constexpr const char* EventAssetsUpload = "AssetsUpload";
	constexpr const char* EventNodesChanged = "NodesChanged";
	
	class Project final
	{
	public:
		EventTower eventTower;
		std::unordered_map<std::string, std::any> settings;

	public:
		static Project* singleton();

		bool newProj(const std::string& path);
		bool open(const std::string& path);
		void close();
		void save();

		bool addModel(const std::string& path, Node* root);
		bool setHDRI(const std::string& path);
		bool importGltf(const std::string& path);

		inline void init(const std::shared_ptr<Device>& device) { _device = device; }
		inline void deinit() { _device.reset(); }
		inline bool isOpened() { return _scene != nullptr; }
		inline std::string getRootPath() { return _path; }
		inline Scene* getScene() { return _scene.get(); }

	private:
		std::shared_ptr<Device> _device;
		std::string _path;
		std::unique_ptr<Scene> _scene;
	};
}
