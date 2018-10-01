#include "mcv_platform.h"
#include "module_scene_manager.h"
#include "handle/handle.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "modules/game/module_game_manager.h"
#include "components/comp_group.h"
#include "components/lighting/comp_light_point.h"
#include "components/comp_render.h"
#include "components/comp_transform.h"
#include "render/texture/material.h"
#include "render/render_objects.h"
#include <thread>
#include <fstream>
#include "resources/json_resource.h"
#include "resources/resources_manager.h"
#include <future>

// for convenience
using json = nlohmann::json;

CModuleSceneManager::CModuleSceneManager(const std::string& name)
    : IModule(name)
{}

/* Pre-load all the scenes from boot.json */
void CModuleSceneManager::loadJsonScenes(const std::string filepath) {

    sceneCount = 0;

    json jboot = loadJson(filepath);
    _default_scene = jboot.value("default_scene","scene_intro");

    for (auto it = std::next(jboot.begin(),1); it != jboot.end(); ++it) {
   
        sceneCount++;
        std::string scene_name = it.key();
        std::vector< std::string > groups_subscenes = jboot[scene_name]["scene_group"];
        
        // Create the scene and store it
        Scene * scene = createScene(scene_name);
        scene->groups_subscenes = groups_subscenes;
        auto& data = jboot[scene_name]["static_data"];
        scene->navmesh = data.value("navmesh", "");
        scene->initial_script_name = data.value("initial_script", "");

        scene->env_fog = data.count("env_fog") ? loadVEC3(data["env_fog"]) : cb_globals.global_fog_env_color;
        scene->ground_fog = data.count("ground_fog") ? loadVEC3(data["ground_fog"]) : cb_globals.global_fog_color;
        scene->shadow_color = data.count("shadow_color") ? loadVEC3(data["shadow_color"]) : cb_globals.global_shadow_color;

        scene->env_fog_density = data.value("env_fog_density", cb_globals.global_fog_density);
        scene->ground_fog_density = data.value("ground_fog_density", cb_globals.global_fog_ground_density);

        scene->scene_exposure = data.value("exposure", cb_globals.global_exposure_adjustment);
        scene->scene_ambient = data.value("ambient", cb_globals.global_ambient_adjustment);
        scene->scene_gamma = data.value("gamma", cb_globals.global_gamma_correction_enabled);
        scene->scene_tone_mapping = data.value("tone_mapping", cb_globals.global_tone_mapping_mode);
        scene->scene_shadow_intensity = data.value("shadow_intensity", cb_globals.global_shadow_intensity);


        _scenes.insert(std::pair<std::string, Scene*>(scene_name, scene));
    }
}

bool CModuleSceneManager::parseSceneResources(const std::string& filename, TEntityParseContext& ctx) {
	ctx.filename = filename;

	const json& j_scene = Resources.get(filename)->as<CJsonResource>()->getJson();
	assert(j_scene.is_array());

	// For each item in the array...
	for (int i = 0; i < j_scene.size(); ++i) {
		auto& j_item = j_scene[i];

		assert(j_item.is_object());

		if (j_item.count("entity")) {
			auto& j_entity = j_item["entity"];

			// Do we have the prefab key in the json?
			if (j_entity.count("prefab")) {

				// Get the src/id of the prefab
				std::string prefab_src = j_entity["prefab"];
				assert(!prefab_src.empty());

				// Parse the prefab, if any other child is created they will inherit our ctx transform
				TEntityParseContext prefab_ctx(ctx);
				if (!parseSceneResources(prefab_src, prefab_ctx))
					return false;

			}
			else {
				for (auto& j_current = j_entity.begin(); j_current != j_entity.end(); j_current++) {
					std::vector<std::string>::iterator it;

					if (j_entity.count("render") > 0) {
						auto& j_render = j_entity["render"];
						for (auto ite = j_render.begin(); ite != j_render.end(); ++ite) {

							if (ite.value().count("mesh")) {
								std::string mesh = ite.value().value("mesh", "");
								//assert(mesh != "");
								it = std::find(_resources.begin(), _resources.end(), mesh);
								if (it == _resources.end() && mesh != "") {

									_resources.emplace_back(mesh);
								}
							}
							if (ite.value().count("materials") > 0) {
								std::vector<std::string> j_materials = ite.value()["materials"];
								for (auto material : j_materials) {
									//assert(material != "");
									it = std::find(_resources.begin(), _resources.end(), material);
									if (it == _resources.end() && material != "") {

										_resources.emplace_back(material);
									}
								}
							}
						}
					}
					if (j_entity.count("fade_controller") > 0) {
						auto& j_fadeController = j_entity["fade_controller"];
						std::string material = j_fadeController.value("material", "");
						//assert(material != "");
						it = std::find(_resources.begin(), _resources.end(), material);
						if (it == _resources.end() && material != "") {

							_resources.emplace_back(material);
						}
					}
					if (j_entity.count("skeleton") > 0) {
						auto& j_skeleton = j_entity["skeleton"];
						std::string skeleton = j_skeleton.value("skeleton", "");
						//assert(skeleton != "");
						it = std::find(_resources.begin(), _resources.end(), skeleton);
						if (it == _resources.end() && skeleton != "") {

							_resources.emplace_back(skeleton);
						}
					}
					if (j_entity.count("fsm") > 0) {
						auto& j_fsm = j_entity["fsm"];
						std::string file = j_fsm.value("file", "");
						//assert(file != "");
						it = std::find(_resources.begin(), _resources.end(), file);
						if (it == _resources.end() && file != "") {

							_resources.emplace_back(file);
						}
					}
					if (j_entity.count("particles") > 0) {
						auto& j_particles = j_entity["particles"];
						if (j_particles.count("cores") > 0) {
							std::vector<std::string> j_cores = j_particles["cores"];
							for (auto core : j_cores) {
								//assert(core != "");
								it = std::find(_resources.begin(), _resources.end(), core);
								if (it == _resources.end() && core != "") {

									_resources.emplace_back(core);
								}
							}
						}
					}
					if (j_entity.count("color_grading") > 0) {
						auto& j_colorGrading = j_entity["color_grading"];
						std::string lut = j_colorGrading.value("lut", "");
						//assert(lut != "");
						it = std::find(_resources.begin(), _resources.end(), lut);
						if (it == _resources.end() && lut != "") {

							_resources.emplace_back(lut);
						}
					}
					if (j_entity.count("collider") > 0) {
						auto& j_collider = j_entity["collider"];
						if (j_collider.count("name") > 0) {
							std::string collider = j_collider.value("name", "");
							//assert(collider != "");
							it = std::find(_resources.begin(), _resources.end(), collider);
							if (it == _resources.end() && collider != "") {

								_resources.emplace_back(collider);
							}
						}

					}

				}

			}
		}
	}
	return true;
}

bool CModuleSceneManager::generateResourceLists() {
	auto it = _scenes.begin();
	while (it != _scenes.end()) {
		std::string filename = "data/scenes/Resource List " + it->second->name + ".txt";
		Scene * current_scene = it->second;
		for (auto& scene_name : current_scene->groups_subscenes) {
			TEntityParseContext ctx;
			if (!parseSceneResources(scene_name, ctx)) {
				return false;
			}
		}
		//std::ofstream file{ "data/scenes/Test.resources" };
		std::ofstream file(filename, std::ofstream::out);
		for (auto line : _resources) {
			//file.write(line.c_str(), line.size());
			file << line;
			file << std::endl;
		}
		it++;
	}
	_resources.clear();
	return true;
}

bool CModuleSceneManager::start() {

    // Load a persistent scene and the listed ones
    // Store at persistent scene, inviolable data.
    _persistentScene = createScene("Persistent_Scene");
    _persistentScene->isLoaded = true;

    loadJsonScenes("data/boot.json");
	EngineMultithreading.fut = std::async(std::launch::async, [&] {return generateResourceLists(); });

    return true;
}

bool CModuleSceneManager::stop() {

    unLoadActiveScene();

    return true;
}

void CModuleSceneManager::update(float delta) {

	if (preparingLevel) {
		createResources();
	}
}

Scene* CModuleSceneManager::createScene(const std::string& name) {

    Scene* scene = new Scene();
    scene->name = name;
    scene->navmesh = "UNDEFINED";
    scene->initial_script_name = "UNDEFINED";
    scene->isLoaded = false;

    return scene;
}

/* Method used to load a listed scene (must be in the database) */
bool CModuleSceneManager::loadScene(const std::string & name) {

    auto it = _scenes.find(name);
    if (it != _scenes.end())
    {
        // Send a message to notify the scene loading.
        // Useful if we want to show a load splash menu

        if (_activeScene != nullptr && _activeScene->name != name) {
            /* If the new scene is different from the actual one => delete checkpoint */
            CModuleGameManager gameManager = CEngine::get().getGameManager();
            gameManager.deleteCheckpoint();
        }

        unLoadActiveScene();

        // Load the subscene
        Scene * current_scene = it->second;
        if (current_scene->navmesh.compare("") != 0) {
            Engine.getNavmeshes().buildNavmesh(current_scene->navmesh);
        }

        for (auto& scene_name : current_scene->groups_subscenes) {
            dbg("Autoloading scene %s\n", scene_name.c_str());
            TEntityParseContext ctx;
            parseScene(scene_name, ctx);
			for (auto& entity : ctx.entities_loaded) {
				CEntity* e = entity;
				_entitiesLoaded.push_back(e->getName());
			}
        }

		//Creating file with the entities of the level
		std::string filename = "data/scenes/Entities List " + it->second->name + ".txt";
		std::ofstream file(filename, std::ofstream::out);
		for (auto line : _entitiesLoaded) {
			//file.write(line.c_str(), line.size());
			file << line;
			file << std::endl;
		}

		_entitiesLoaded.clear();

        // Renew the active scene
        current_scene->isLoaded = true;
        setActiveScene(current_scene);

        // Move this to LUA.
        CHandle h_camera = getEntityByName("TPCamera");
        if (h_camera.isValid())
            Engine.getCameras().setDefaultCamera(h_camera);

        h_camera = getEntityByName("main_camera");
        if (h_camera.isValid())
            Engine.getCameras().setOutputCamera(h_camera);

        auto om = getObjectManager<CEntity>();
        om->forEach([](CEntity* e) {
            TMsgSceneCreated msg;
            CHandle h_e(e);
            h_e.sendMsg(msg);
        });

		CModuleGameManager gameManager = CEngine::get().getGameManager();
		/* TODO: Comprobar que se sigue en la misma escena */
		gameManager.loadCheckpoint();
        Engine.getLogic().execEvent(EngineLogic.SCENE_START, current_scene->name);

        // Set the global data.
        cb_globals.global_fog_color = current_scene->ground_fog;
        cb_globals.global_fog_env_color = current_scene->env_fog;
        cb_globals.global_fog_density = current_scene->env_fog_density;
        cb_globals.global_fog_ground_density = current_scene->ground_fog_density;
        cb_globals.global_exposure_adjustment = current_scene->scene_exposure;
        cb_globals.global_ambient_adjustment = current_scene->scene_ambient;
        cb_globals.global_gamma_correction_enabled = current_scene->scene_gamma;
        cb_globals.global_tone_mapping_mode = current_scene->scene_tone_mapping;
        cb_globals.global_shadow_intensity = current_scene->scene_shadow_intensity;
        cb_globals.global_shadow_color = current_scene->shadow_color;
        cb_globals.updateGPU();

        return true;
    }

    return false;
}

bool CModuleSceneManager::unloadScene(const std::string & name) {
	
	getEntitiesList(name);
	for (auto& entity : _entitiesLoaded) {
		CHandle h = getEntityByName(entity);
		if (h.isValid()) {
			CEntity * e = h;
			e->~CEntity();
		}

	}

	//EngineLogic.clearDelayedScripts();
	//EngineLogic.execEvent(EngineLogic.SCENE_END, _activeScene->name);

	//EngineEntities.destroyAllEntities();
	//EngineCameras.deleteAllCameras();
	//EngineIA.clearSharedBoards();
	//EngineNavmeshes.destroyNavmesh();
	//EngineInstancing.clearInstances();
	//EngineParticles.killAll();

	//_activeScene->isLoaded = false;
	//_activeScene = nullptr;

	///* TODO: Delete checkpoint */

	return true;
}


/* Method used to retrieve resources needed to load a level */
bool CModuleSceneManager::getResourcesList(const std::string & name) {
	std::string path = "data/scenes/";
	std::string filename = path + "Resource List " + name + ".txt";
	std::ifstream file(filename);
	std::string str;
	while (std::getline(file, str))
	{
		// Process str
		_resources.emplace_back(str);
	}
	preparingLevel = true;
	levelToLoad = name;
	return true;
}

/* Method used to retrieve entities needed to unload a level */
bool CModuleSceneManager::getEntitiesList(const std::string & name) {
	std::string path = "data/scenes/";
	std::string filename = path + "Entities List " + name + ".txt";
	std::ifstream file(filename);
	std::string str;
	while (std::getline(file, str))
	{
		// Process str
		_entitiesLoaded.emplace_back(str);
	}
	return true;
}

void CModuleSceneManager::createResources() {
	int step = 5;
	int i = 0;
	while (i < step && !_resources.empty()) {
		Resources.get(_resources[0]);
		_resources.erase(_resources.begin());
		i++;
	}
	if (_resources.empty()) {
		preparingLevel = false;
		PROFILE_SET_NFRAMES(1);
		PROFILE_FRAME_BEGINS();
		loadScene(levelToLoad);
		PROFILE_FRAME_BEGINS();
		levelToLoad.clear();
	}
}

bool CModuleSceneManager::unLoadActiveScene() {

    // This will allow us to mantain the gamestate.

    // Get the current active scene
    // Free memory related to non persistent data.
    // Warning: persistent data will need to avoid deletion
    if (_activeScene != nullptr) {

        EngineLogic.clearDelayedScripts();
        EngineLogic.execEvent(EngineLogic.SCENE_END, _activeScene->name);

        EngineEntities.destroyAllEntities();
        EngineCameras.deleteAllCameras();
        EngineIA.clearSharedBoards();
        EngineNavmeshes.destroyNavmesh();
        EngineInstancing.clearInstances();
        EngineParticles.killAll();

        _activeScene->isLoaded = false;
        _activeScene = nullptr;

	    /* TODO: Delete checkpoint */

        return true;
    }

    return false;
}

/* Some getters and setters */

Scene* CModuleSceneManager::getActiveScene() {

    return _activeScene;
}

Scene* CModuleSceneManager::getSceneByName(const std::string& name) {

    return _scenes[name];
}

void CModuleSceneManager::setActiveScene(Scene* scene) {

    //unLoadActiveScene();
    _activeScene = scene;
}

std::string CModuleSceneManager::getDefaultSceneName() {

    return _default_scene;
}