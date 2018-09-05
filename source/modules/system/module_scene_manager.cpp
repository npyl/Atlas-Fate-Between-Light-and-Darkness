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
#include <thread>
#include <fstream>
#include "resources/json_resource.h"


// for convenience
using json = nlohmann::json;

CModuleSceneManager::CModuleSceneManager(const std::string& name)
	: IModule(name)
{}

/* Pre-load all the scenes from boot.json */
void CModuleSceneManager::loadJsonScenes(const std::string filepath) {

	sceneCount = 0;

	json jboot = loadJson(filepath);
	_default_scene = jboot.value("default_scene", "scene_intro");

	for (auto it = std::next(jboot.begin(), 1); it != jboot.end(); ++it) {

		sceneCount++;
		std::string scene_name = it.key();
		std::vector< std::string > groups_subscenes = jboot[scene_name]["scene_group"];

		// Create the scene and store it
		Scene * scene = createScene(scene_name);
		scene->groups_subscenes = groups_subscenes;
		auto& data = jboot[scene_name]["static_data"];
		scene->navmesh = data.value("navmesh", "data/navmeshes/milestone2_navmesh.bin");
		scene->initial_script_name = data.value("initial_script", "");

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
								assert(mesh != "");
								it = std::find(_resources.begin(), _resources.end(), mesh);
								if (it == _resources.end()) {
									_resources.emplace_back(mesh);
								}
							}
							if (ite.value().count("materials") > 0) {
								std::vector<std::string> j_materials = ite.value()["materials"];
								for (auto material : j_materials) {
									assert(material != "");
									it = std::find(_resources.begin(), _resources.end(), material);
									if (it == _resources.end()) {
										_resources.emplace_back(material);
									}
								}
							}
						}
					}
					if (j_entity.count("fade_controller") > 0) {
						auto& j_fadeController = j_entity["fade_controller"];
						std::string material = j_fadeController.value("material", "");
						assert(material != "");
						it = std::find(_resources.begin(), _resources.end(), material);
						if (it == _resources.end()) {
							_resources.emplace_back(material);
						}
					}
					if (j_entity.count("skeleton") > 0) {
						auto& j_skeleton = j_entity["skeleton"];
						std::string skeleton = j_skeleton.value("skeleton", "");
						assert(skeleton != "");
						it = std::find(_resources.begin(), _resources.end(), skeleton);
						if (it == _resources.end()) {
							_resources.emplace_back(skeleton);
						}
					}
					if (j_entity.count("fsm") > 0) {
						auto& j_fsm = j_entity["fsm"];
						std::string file = j_fsm.value("file", "");
						assert(file != "");
						it = std::find(_resources.begin(), _resources.end(), file);
						if (it == _resources.end()) {
							_resources.emplace_back(file);
						}
					}
					if (j_entity.count("particles") > 0) {
						auto& j_particles = j_entity["particles"];
						if (j_particles.count("cores") > 0) {
							std::vector<std::string> j_cores = j_particles["cores"];
							for (auto core : j_cores) {
								assert(core != "");
								it = std::find(_resources.begin(), _resources.end(), core);
								if (it == _resources.end()) {
									_resources.emplace_back(core);
								}
							}
						}
					}
					if (j_entity.count("color_grading") > 0) {
						auto& j_colorGrading = j_entity["color_grading"];
						std::string lut = j_colorGrading.value("lut", "");
						assert(lut != "");
						it = std::find(_resources.begin(), _resources.end(), lut);
						if (it == _resources.end()) {
							_resources.emplace_back(lut);
						}
					}
					if (j_entity.count("collider") > 0) {
						auto& j_collider = j_entity["collider"];
						if (j_collider.count("name") > 0) {
							std::string collider = j_collider.value("name", "");
							assert(collider != "");
							it = std::find(_resources.begin(), _resources.end(), collider);
							if (it == _resources.end()) {
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

void CModuleSceneManager::generateResourceLists() {
	auto it = _scenes.begin();
	while (it != _scenes.end()) {
		std::string filename = "data/scenes/Resource List " + it->second->name + ".txt";
		Scene * current_scene = it->second;
		for (auto& scene_name : current_scene->groups_subscenes) {
			TEntityParseContext ctx;
			parseSceneResources(scene_name, ctx);
		}
		//std::ofstream file{ "data/scenes/Test.resources" };
		std::ofstream file(filename, std::ofstream::out);
		for (auto line : _resources) {
			//file.write(line.c_str(), line.size());
			file << line;
			file << std::endl;
		}
		//_resources;
		it++;
	}
}

bool CModuleSceneManager::start() {

	// Load a persistent scene and the listed ones
	// Store at persistent scene, inviolable data.
	_persistentScene = createScene("Persistent_Scene");
	_persistentScene->isLoaded = true;

	loadJsonScenes("data/boot.json");
	generateResourceLists();

	return true;
}

bool CModuleSceneManager::stop() {

	unLoadActiveScene();

	return true;
}

void CModuleSceneManager::update(float delta) {

	// TO-DO, Maybe not needed
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
		Engine.getNavmeshes().buildNavmesh(current_scene->navmesh);

		for (auto& scene_name : current_scene->groups_subscenes) {
			dbg("Autoloading scene %s\n", scene_name.c_str());
			TEntityParseContext ctx;
			parseScene(scene_name, ctx);
		}

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
		Engine.getLogic().execEvent(EngineLogic.SCENE_START, current_scene->initial_script_name);

		// TO REMOVE.
		// Guarrada maxima color neones
		{
			CHandle p_group = getEntityByName("neones");
			CEntity * parent_group = p_group;
			if (p_group.isValid()) {
				TCompGroup * neon_group = parent_group->get<TCompGroup>();
				for (auto p : neon_group->handles) {
					CEntity * neon = p;
					TCompTransform * t_trans = neon->get<TCompTransform>();
					VEC3 pos = t_trans->getPosition();
					CEntity * to_catch = nullptr;
					float maxDistance = 9999999;
					getObjectManager<TCompLightPoint>()->forEach([pos, &to_catch, &maxDistance](TCompLightPoint* c) {
						CEntity * ent = CHandle(c).getOwner();
						TCompTransform * c_trans = ent->get<TCompTransform>();
						float t_distance = VEC3::Distance(pos, c_trans->getPosition());

						if (t_distance < maxDistance) {
							to_catch = ent;
							maxDistance = t_distance;
						}
					});

					if (to_catch != nullptr) {
						TCompLightPoint * point_light = to_catch->get<TCompLightPoint>();
						VEC4 neon_color = point_light->getColor();
						TCompRender * l_render = neon->get<TCompRender>();
						l_render->self_color = neon_color;
						l_render->self_intensity = 10.f;
						/*for (auto p : l_render->meshes) {
							for (auto t : p.materials) {
								CMaterial * mat = const_cast<CMaterial*>(t);
								mat->setSelfColor(VEC4(1,0,0,1));
								dbg("changed color");
							}
						}*/
					}
				}
			}
		}

		return true;
	}

	return false;
}

/* Method used to load a listed scene (must be in the database) */
bool CModuleSceneManager::prepareSceneMT(const std::string & name) {

	auto it = _scenes.find(name);
	if (it != _scenes.end())
	{

		// Load the subscene
		Scene * current_scene = it->second;

		for (auto& scene_name : current_scene->groups_subscenes) {
			dbg("Preparing scene to load scene %s\n", scene_name.c_str());
			TEntityParseContext ctx;
			parseScene(scene_name, ctx);
		}
		return true;
	}

	return false;
}

bool CModuleSceneManager::preparingSceneMT(const std::string & name) {
	bool done = false;
	//std::thread loadSceneThread(&CModuleSceneManager::prepareSceneMT, name);
	//loadSceneThread.join();
	//dbg("Scene prepared: %s \n \n \n \n \n", name.c_str());
	Sleep(4000);
	dbg("4 seconds waited");
	Sleep(2000);
	dbg("2 more seconds waited and i am out");
	done = true;
	return done;
}


//void CModuleSceneManager::loadPreparedSceneMT() {
//
//}

bool CModuleSceneManager::unLoadActiveScene() {

	// This will allow us to mantain the gamestate.

	// Get the current active scene
	// Free memory related to non persistent data.
	// Warning: persistent data will need to avoid deletion
	if (_activeScene != nullptr) {

		EngineEntities.destroyAllEntities();
		EngineCameras.deleteAllCameras();
		EngineIA.clearSharedBoards();
		EngineNavmeshes.destroyNavmesh();
		EngineInstancing.clearInstances();
		EngineParticles.killAll();

		Engine.getLogic().execEvent(EngineLogic.SCENE_END, _activeScene->name);

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