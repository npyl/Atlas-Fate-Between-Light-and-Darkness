#include "mcv_platform.h"
#include "entity/entity_parser.h"
#include "comp_cone_of_light.h"
#include "components/comp_transform.h"
#include "components/comp_tags.h"
#include "components/comp_render.h"
#include "components/player_controller/comp_player_tempcontroller.h"
#include "components/lighting/comp_light_spot.h"
#include "components/comp_hierarchy.h"
#include "render/mesh/mesh_loader.h"
#include "render/render_objects.h"

DECL_OBJ_MANAGER("cone_of_light", TCompConeOfLightController);

void TCompConeOfLightController::debugInMenu() {
}

void TCompConeOfLightController::load(const json& j, TEntityParseContext& ctx) {
	hor_fov = deg2rad(j.value("fov", 70.f));
	ver_fov = deg2rad(j.value("fov", 45.f));
	dist = j.value("dist", 10.f);
	player = getEntityByName(j.value("target", "The Player"));
	turnedOn = j.value("turnedOn", false);
}

void TCompConeOfLightController::registerMsgs() {
	DECL_MSG(TCompConeOfLightController, TMsgEntityCreated, onMsgEntityCreated);
}

void TCompConeOfLightController::onMsgEntityCreated(const TMsgEntityCreated& msg) {
	if (!turnedOn) {
		TCompLightSpot * spotlight = get<TCompLightSpot>();
		spotlight->isEnabled = false;
	}
}

void TCompConeOfLightController::update(float dt) {
	bool isPlayerIlluminatedNow = false;
	if (turnedOn) {
		TCompTempPlayerController* pController = player->get<TCompTempPlayerController>();
		TCompTransform* ppos = player->get<TCompTransform>();
		TCompTransform* mypos = get<TCompTransform>();
		bool inDist = VEC3::Distance(mypos->getPosition(), ppos->getPosition()) < dist;
		if (VEC3::Distance(mypos->getPosition(), ppos->getPosition()) < dist
			&& mypos->isInFov(ppos->getPosition(), hor_fov, ver_fov)) {
			if (!isPlayerHiddenFromLight(player)) {
				isPlayerIlluminatedNow = true;
				if (!playerIlluminated) {
					playerIlluminated = true;
					TMsgPlayerIlluminated msg;
					TCompHierarchy *tHierarchy = get<TCompHierarchy>();
					msg.h_sender = tHierarchy->h_parent;
					msg.isIlluminated = true;
					player->sendMsg(msg);
				}
			}
		}
	}

	if (playerIlluminated && !isPlayerIlluminatedNow) {
		playerIlluminated = false;
		TMsgPlayerIlluminated msg;
		TCompHierarchy *tHierarchy = get<TCompHierarchy>();
		msg.h_sender = tHierarchy->h_parent;
		msg.isIlluminated = false;
		player->sendMsg(msg);
	}
}

void TCompConeOfLightController::turnOnLight() {
	if (!turnedOn) {
		TCompLightSpot * spotlight = get<TCompLightSpot>();
		spotlight->isEnabled = true;
		turnedOn = true;
	}
}

void TCompConeOfLightController::turnOffLight() {
	if (turnedOn) {
		TCompLightSpot * spotlight = get<TCompLightSpot>();
		spotlight->isEnabled = false;
		turnedOn = false;
	}
}

bool TCompConeOfLightController::isPlayerHiddenFromLight(CEntity* player)
{
	CEntity* parent = CHandle(this).getOwner().getOwner();
	TCompTransform *mypos = parent->get<TCompTransform>();
	TCompTransform *pTransform = player->get<TCompTransform>();
	TCompCollider *myCollider = parent->get<TCompCollider>();
	TCompCollider *pCollider = player->get<TCompCollider>();

	VEC3 myPosition = mypos->getPosition();

	origin = myPosition + VEC3(0, 3 + .1f, 0);
	dest = pTransform->getPosition() + VEC3(0, .1f, 0);
	VEC3 dir = dest - origin;
	dir.Normalize();
	float distance = VEC3::Distance(origin, dest);

	physx::PxRaycastHit hit;

	//TODO: only works when behind scenery. Make the same for other enemies, dynamic objects...
	return EnginePhysics.Raycast(origin, dir, distance, hit, physx::PxQueryFlag::eSTATIC);
}