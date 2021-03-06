#include "mcv_platform.h"
#include "comp_bone_tracker.h"
#include "components/comp_transform.h"
#include "comp_skeleton.h"
#include "cal3d/cal3d.h"
#include "cal3d2engine.h"
#include "entity/entity_parser.h"

DECL_OBJ_MANAGER("bone_tracker", TCompBoneTracker);

void TCompBoneTracker::load(const json& j, TEntityParseContext& ctx) {

    bone_name = j.value("bone", "");
    parent_name = j.value("parent", "");
    rot_update = j.value("rotation_update", true);
    assert(!bone_name.empty());
    assert(!parent_name.empty());

	rot_offset = QUAT::Identity;
	if (j.count("rotation_offset_axis")) {
		VEC3 rot_offset_axis = loadVEC3(j["rotation_offset_axis"]);
		float angle_deg = j.value("angle", 0.f);
		float angle_rad = deg2rad(angle_deg);
		rot_offset = QUAT::CreateFromAxisAngle(rot_offset_axis, angle_rad);
	}

    CEntity* e_parent = ctx.findEntityByName(parent_name);
    if (e_parent)
        h_skeleton = e_parent->get<TCompSkeleton>();
}

void TCompBoneTracker::update(float dt) {

    if (!CHandle(this).getOwner().isValid())
        return;

    TCompSkeleton* c_skel = h_skeleton;

    if (c_skel == nullptr) {
        // Search the parent entity by name
        CEntity* e_entity = getEntityByName(parent_name);
        if (!e_entity)
            return;
        // The entity I'm tracking should have an skeleton component.
        h_skeleton = e_entity->get<TCompSkeleton>();
        assert(h_skeleton.isValid());
        c_skel = h_skeleton;
    }

    if (bone_id == -1) {
        bone_id = c_skel->model->getCoreModel()->getCoreSkeleton()->getCoreBoneId(bone_name);
        // The skeleton don't have the bone with name 'bone_name'
        assert(bone_id != -1);
    }

    // Access to the bone 'bone_id' of the skeleton
    auto cal_bone = c_skel->model->getSkeleton()->getBone(bone_id);
    QUAT rot = rot_offset * Cal2DX(cal_bone->getRotationAbsolute());
    VEC3 pos = Cal2DX(cal_bone->getTranslationAbsolute());

    // Apply the cal3d pos&rot to my entity owner
    TCompTransform* tmx = get<TCompTransform>();
    if (tmx) {
        tmx->setPosition(pos);
        if(rot_update) tmx->setRotation(rot);
    }
}
