#include "mcv_platform.h"
#include "components/comp_transform.h"
#include "utils/utils.h"
#include "components/camera_controller/comp_camera_front_transform.h"

DECL_OBJ_MANAGER("comp_camera_front_transform", TCompCameraFrontTransform);

void TCompCameraFrontTransform::debugInMenu() {

}

void TCompCameraFrontTransform::load(const json& j, TEntityParseContext& ctx) {
	target_transorm_name = j.value("target_name", "");
}

void TCompCameraFrontTransform::update(float dt) {
	if (target_transform == nullptr) {
		target_transform = getEntityByName(target_transorm_name);
	}
	if (player_transform == nullptr) {
		player_transform = getEntityByName("The Player");
	}
	
	TCompTransform * c_point_transform = target_transform->get<TCompTransform>();
	TCompTransform * c_player_transform = player_transform->get<TCompTransform>();
	TCompTransform * c_camera_transform = get<TCompTransform>();
	Vector3 player_front = c_player_transform->getFront();
	Vector3 camera_front = c_camera_transform->getFront();
	float factor = player_front.Dot(camera_front);
	float angle = cosf(factor);
	dbg("%f		%f\n",factor,  angle);

	c_point_transform->setPosition(c_camera_transform->getPosition() + c_camera_transform->getFront() * 10);
	Vector3 prova = Vector3(-400, 500, 0);
	prova.Clamp(Vector3(-100, -100, -100), Vector3(100, 100,100 ));
	//dbg("%f    %f    %f\n", prova.x, prova.y, prova.z);
	
}
