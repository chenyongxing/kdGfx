#include "Project.h"
#include "SceneLoader.h"

#include <json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

#define PROJECT_VERSION "1.0.0";

namespace kdGfx
{
	Project* Project::singleton()
	{
		static Project _singleton;
		return &_singleton;
	}

	bool Project::newProj(const std::string& path)
	{
		if (!fs::is_empty(path)) return false;

		_path = path;
		_scene = std::make_unique<Scene>();
		_scene->init(_device);
		_scene->markAssetsDirty();
		_scene->markDirty();

		std::ofstream ofs(std::string(path).append("/project.json"));
		if (ofs.is_open())
		{
			json projectJson;
			projectJson["version"] = PROJECT_VERSION;
			ofs << std::setw(4) << projectJson << std::endl;
			ofs.close();
		}

		fs::create_directory(std::string(path).append("/HDRIs"));
		fs::create_directory(std::string(path).append("/Materials"));
		fs::create_directory(std::string(path).append("/Models"));
		
		SceneLoader sceneLoader;
		sceneLoader.save();

		return true;
	}

	bool Project::open(const std::string& path)
	{
		_path = path;
		_scene = std::make_unique<Scene>();
		_scene->init(_device);

		auto startTime = std::chrono::steady_clock::now();
		SceneLoader sceneLoader;
		if (!sceneLoader.load())	return false;
		auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<float> timeDura = endTime - startTime;
		spdlog::info("scene load {}. cost {}s", path, timeDura.count());

		return true;
	}

	void Project::close()
	{
		if (!_scene) return;

		_scene->destroy();
		_scene.reset();
	}

	void Project::save()
	{
		if (!_scene) return;

		std::ofstream ofs(std::string(_path).append("/project.json"));
		if (ofs.is_open())
		{
			json settingsJson;

			json projectJson;
			projectJson["version"] = PROJECT_VERSION;
			projectJson["settings"] = settingsJson;
			ofs << std::setw(4) << projectJson << std::endl;
			ofs.close();
		}

		SceneLoader sceneLoader;
		sceneLoader.save();
	}

	bool Project::addModel(const std::string& path, Node* root)
	{
		if (!_scene) return false;

		SceneLoader sceneLoader;
		return sceneLoader.addModel(path, root);
	}

	bool Project::setHDRI(const std::string& path)
	{
		if (!_scene) return false;

		SceneLoader sceneLoader;
		return sceneLoader.setHDRI(path);
	}

	bool Project::importGltf(const std::string& path)
	{
		return false;
	}
}
