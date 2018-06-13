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
	TCompTransform * c_point_transform = target_transform->get<TCompTransform>();
	TCompTransform * c_transform = get<TCompTransform>();
	c_point_transform->setPosition(c_transform->getPosition() + c_transform->getFront() * 10);
}
