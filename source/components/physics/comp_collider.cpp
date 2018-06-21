#include "mcv_platform.h"
#include "comp_collider.h"
#include "comp_rigidbody.h"
#include "components/comp_transform.h"
#include "physics/physics_collider.h"
#include "modules\system\module_logic.h"
#include "components\comp_tags.h"
#include "render/texture/material.h"
#include "render/render_objects.h"
#include "utils\utils.h"


DECL_OBJ_MANAGER("collider", TCompCollider);

TCompCollider::~TCompCollider(){

	// In case it's a controller, delegate it's destruction to the rigidbody.
	if (config->actor != nullptr){

		if (!config->is_controller) {
			config->actor->release();
			config->actor = nullptr;
		}
	}
}

void TCompCollider::debugInMenu() {

	config->debugInMenu();
}

void TCompCollider::renderDebug(bool onlyDynamics) {
	activateRSConfig(RSCFG_WIREFRAME);
	renderColliders(onlyDynamics);
	activateRSConfig(RSCFG_DEFAULT);
}

void TCompCollider::renderColliders(bool onlyDynamics) {
	const CMaterial* material = Resources.get("data/materials/solid.material")->as<CMaterial>();
	auto mesh = Resources.get("wired_unit_cube.mesh")->as<CRenderMesh>();
	CEntity* e = CHandle(this).getOwner();
	std::string name_trigger = e->getName();
	TCompTransform* mypos = get<TCompTransform>();
	if (!mypos)
		return;
	CTransform pos = *mypos;

	//If the component has a shape
	if (config->shape && !onlyDynamics) {

		if (physx::PxGeometryType::eBOX == this->config->shape->getGeometryType()) {
			physx::PxBoxGeometry box;
			this->config->shape->getBoxGeometry(box);
			VEC3 Extents = VEC3(box.halfExtents.x * 2, box.halfExtents.y * 2, box.halfExtents.z * 2);

			VEC3 p = VEC3(pos.getPosition().x, pos.getPosition().y + box.halfExtents.y, pos.getPosition().z);
			pos.setPosition(p);
			MAT44 m = MAT44::CreateScale(Extents) * pos.asMatrix();
			renderMesh(mesh, m);
		}
		else if (physx::PxGeometryType::eCAPSULE == this->config->shape->getGeometryType()) {
			physx::PxCapsuleGeometry capsule;
			this->config->shape->getCapsuleGeometry(capsule);
			VEC3 Extents = VEC3(capsule.radius * 2, capsule.halfHeight * 2 + capsule.radius * 2, capsule.radius * 2);

			VEC3 p = VEC3(pos.getPosition().x, pos.getPosition().y + capsule.halfHeight + capsule.radius, pos.getPosition().z);
			pos.setPosition(p);
			MAT44 m = MAT44::CreateScale(Extents) * pos.asMatrix();
			renderMesh(mesh, m, VEC4(0, 255, 0, 1));
		}
		else if (physx::PxGeometryType::eCONVEXMESH == this->config->shape->getGeometryType()) {
			//To-Do
		}
	}
	else if (config->is_controller) {	//If the component is a controller, we will retrieve the shape from the actor.
		const physx::PxU32 numShapes = this->config->actor->getNbShapes();
		//assert(numShapes == 1); //At the moment, we are retrieving the shapes as if they were only one. If this changes, we have to update this as well.

		//Retrieving controller shape from the actor
		std::vector<physx::PxShape*> shapes;
		shapes.resize(numShapes);
		this->config->actor->getShapes(&shapes[0], numShapes);
		physx::PxShape* player_shape = shapes[0];

		if (physx::PxGeometryType::eBOX == player_shape->getGeometryType()) {
			physx::PxBoxGeometry box;
			player_shape->getBoxGeometry(box);

			VEC3 Extents = VEC3(box.halfExtents.x * 2, box.halfExtents.y * 2, box.halfExtents.z * 2);
			VEC3 p = VEC3(pos.getPosition().x, pos.getPosition().y + box.halfExtents.y, pos.getPosition().z);
			pos.setPosition(p);
			MAT44 m = MAT44::CreateScale(Extents) * pos.asMatrix();
			renderMesh(mesh, m);
		}
		else if (physx::PxGeometryType::eCAPSULE == player_shape->getGeometryType()) {
			physx::PxCapsuleGeometry capsule;
			player_shape->getCapsuleGeometry(capsule);
			VEC3 Extents = VEC3(capsule.radius * 2, capsule.halfHeight * 2 + capsule.radius * 2, capsule.radius * 2);

			VEC3 p = VEC3(pos.getPosition().x, pos.getPosition().y + capsule.halfHeight + capsule.radius, pos.getPosition().z);
			pos.setPosition(p);
			MAT44 m = MAT44::CreateScale(Extents) * pos.asMatrix();
			renderMesh(mesh, m, VEC4(255, 0, 0, 1));
		}
	}

}

void TCompCollider::load(const json& j, TEntityParseContext& ctx) {

	// Factory pattern inside the json loader.
	std::string shape = j["shape"].get<std::string>();
	if (strcmp("box", shape.c_str()) == 0)
	{
		config = new CPhysicsBox();
	}
	else if (strcmp("sphere", shape.c_str()) == 0)
	{
		config = new CPhysicsSphere();
	}
	else if (strcmp("plane", shape.c_str()) == 0)
	{
		config = new CPhysicsPlane();
	}
	else if (strcmp("capsule", shape.c_str()) == 0)
	{
		config = new CPhysicsCapsule();
	}
	else if (strcmp("convex", shape.c_str()) == 0)
	{
		config = new CPhysicsConvex();
	}
	else if (strcmp("mesh", shape.c_str()) == 0)
	{
		config = new CPhysicsTriangleMesh();
	}

	config->group = getFilterByName(j.value("group", "all"));
	config->mask = getFilterByName(j.value("mask", "all"));
	config->is_trigger = j.value("is_trigger", false);

	if (j.count("center"))
	{
		VEC3 center = loadVEC3(j["center"]);
		config->center = physx::PxVec3(center.x, center.y, center.z);
	}

	config->load(j, ctx);
}

void TCompCollider::registerMsgs() {

	DECL_MSG(TCompCollider, TMsgEntityCreated, onCreate);
	DECL_MSG(TCompCollider, TMsgTriggerEnter, onTriggerEnter);
	DECL_MSG(TCompCollider, TMsgTriggerExit, onTriggerExit);
	DECL_MSG(TCompCollider, TMsgEntityDestroyed, onDestroy);
}

void TCompCollider::onCreate(const TMsgEntityCreated& msg) {

	CEntity* e = CHandle(this).getOwner();
	TCompRigidbody * c_rigidbody = e->get<TCompRigidbody>();
	
	// Let the rigidbody handle the creation if it exists..
	if (c_rigidbody == nullptr)
	{
		TCompTransform * compTransform = e->get<TCompTransform>();

		// Create the shape, the actor and set the user data
		physx::PxShape * shape = config->createShape();
		config->createStatic(shape, compTransform);
		config->actor->userData = CHandle(this).asVoidPtr();
	}
}

void TCompCollider::onDestroy(const TMsgEntityDestroyed & msg)
{
	//delete this;
}

void TCompCollider::onTriggerEnter(const TMsgTriggerEnter& msg) {

    CEntity* e = CHandle(this).getOwner();
    std::string trigger_name = e->getName();
	std::map<uint32_t, TCompTransform*>::iterator it = handles.begin();
	uint32_t ext_index = msg.h_other_entity.getExternalIndex();
	if (handles.find(ext_index) == handles.end()){
		CEntity * c_other = msg.h_other_entity;
		TCompCollider * c_collider = c_other->get<TCompCollider>();
		TCompTransform * c_transform = c_other->get<TCompTransform>();

        TCompTags* tags = c_other->get<TCompTags>();
		assert(c_transform);

		handles[ext_index] = c_transform;

		if (c_collider->config->group & FilterGroup::Player)
		{
			player_inside = true;
		}

        if (tags) {
            if (tags->hasTag(getID("player"))) {
                std::string params = trigger_name + "_player()";
                EngineLogic.execEvent(CModuleLogic::Events::TRIGGER_ENTER, params);
            }
            else {
                std::string other_name = c_other->getName();
                std::string params = trigger_name + "_enemy(" + other_name + ")";
                EngineLogic.execEvent(CModuleLogic::Events::TRIGGER_ENTER, params);
            }
        }
	}

	// Get all entities with given tag to test with
	// Send a message to all of them
}

void TCompCollider::onTriggerExit(const TMsgTriggerExit& msg) {

    CEntity* e = CHandle(this).getOwner();
    std::string trigger_name = e->getName();

	auto it = handles.find(msg.h_other_entity.getExternalIndex());
	if (it != handles.end())
	{
		handles.erase(it);
		CEntity * c_other = msg.h_other_entity;
        TCompTags* tags = c_other->get<TCompTags>();
		TCompCollider * c_collider = c_other->get<TCompCollider>();

		if (c_collider->config->group & FilterGroup::Player)
		{
			player_inside = false;
		}

        if (tags) {
            if (tags->hasTag(getID("player"))) {
                std::string params = trigger_name + "_player()";
                EngineLogic.execEvent(CModuleLogic::Events::TRIGGER_EXIT, params);
            }
            else {
                std::string other_name = c_other->getName();
                std::string params = trigger_name + "_enemy(" + other_name + ")";
                EngineLogic.execEvent(CModuleLogic::Events::TRIGGER_EXIT, params);
            }
        }
	}
}

void TCompCollider::update(float dt) {


}

bool TCompCollider::collisionDistance(const VEC3 & org, const VEC3 & dir, float maxDistance) {

	physx::PxRaycastHit hit;
	if (EnginePhysics.Raycast(org, dir, maxDistance, hit, physx::PxQueryFlag::eSTATIC)) {
		return hit.distance < maxDistance ? true : false;
	}

	return false;
}

void TCompCollider::setGlobalPose(VEC3 newPos, VEC4 newRotation, bool autowake)
{
  physx::PxTransform transform(physx::PxVec3(newPos.x, newPos.y, newPos.z), physx::PxQuat(newRotation.x, newRotation.y, newRotation.z, newRotation.w));
  config->actor->setGlobalPose(transform, autowake);
}

TCompCollider::result TCompCollider::collisionSweep(VEC3 dir, float distance, physx::PxQueryFlags flags, physx::PxQueryFilterData queryFilterData, Controllers controller) {
	physx::PxCapsuleGeometry* controller_geometry = &physx::PxCapsuleGeometry();
	physx::PxBoxGeometry* my_geometry = &physx::PxBoxGeometry();
	if (controller != Controllers::eReserved) {
		if (controller == Controllers::ePlayer) {
			//Controller calls
			CEntity* player = getEntityByName("The Player");
			TCompCollider* player_collider = player->get<TCompCollider>();
			const physx::PxU32 numShapes = player_collider->config->actor->getNbShapes();
			assert(numShapes == 1); //At the moment we are retrieving the shapes as if they were only one. If this has changed, we have to update this as well.

									//Retrieving controller shape from the actor
			std::vector<physx::PxShape*> shapes;
			shapes.resize(numShapes);
			player_collider->config->actor->getShapes(&shapes[0], numShapes);
			physx::PxShape* player_shape = shapes[0];
			assert(player_shape->getGeometryType() == physx::PxGeometryType::eCAPSULE); //We also assume that the geometry is a capsule.
			player_shape->getCapsuleGeometry(*controller_geometry);

		}
		else if (controller == Controllers::ePatrol) {
			//Controller calls
			CEntity* patrol = getEntityByName("The Enemy 1"); //Retrieving geometry from the first patrol
			TCompCollider* patrol_collider = patrol->get<TCompCollider>();
			const physx::PxU32 numShapes = patrol_collider->config->actor->getNbShapes();
			assert(numShapes == 1); //At the moment we are retrieving the shapes as if they were only one. If this has changed, we have to update this as well.

									//Retrieving controller shape from the actor
			std::vector<physx::PxShape*> shapes;
			shapes.resize(numShapes);
			patrol_collider->config->actor->getShapes(&shapes[0], numShapes);
			physx::PxShape* patrol_shape = shapes[0];
			assert(patrol_shape->getGeometryType() == physx::PxGeometryType::eCAPSULE); //We also assume that the geometry is a capsule.
			patrol_shape->getCapsuleGeometry(*controller_geometry);
		}
		else if (controller == Controllers::eMimetic) {
			//Controller calls
			CEntity* mimetic = getEntityByName("Mimetic 1"); //Retrieving geometry from the first patrol
			TCompCollider* mimetic_collider = mimetic->get<TCompCollider>();
			const physx::PxU32 numShapes = mimetic_collider->config->actor->getNbShapes();
			assert(numShapes == 1); //At the moment we are retrieving the shapes as if they were only one. If this has changed, we have to update this as well.

									//Retrieving controller shape from the actor
			std::vector<physx::PxShape*> shapes;
			shapes.resize(numShapes);
			mimetic_collider->config->actor->getShapes(&shapes[0], numShapes);
			physx::PxShape* mimetic_shape = shapes[0];
			assert(mimetic_shape->getGeometryType() == physx::PxGeometryType::eCAPSULE); //We also assume that the geometry is a capsule.
			mimetic_shape->getCapsuleGeometry(*controller_geometry);
		}
	}
	else {
		TCompCollider* my_collider = get<TCompCollider>();
		physx::PxBoxGeometry box;
		my_collider->config->shape->getBoxGeometry(box);
		my_geometry = &box;
	}

	//this calls
	CHandle my_handle = CHandle(this).getOwner();
	CEntity* my_entity = my_handle;
	TCompTransform* my_transform = my_entity->get<TCompTransform>();
	VEC3 my_pos = my_transform->getPosition();

	//Sweep data
	physx::PxSweepBuffer hit_Buffer;
	physx::PxTransform my_pxtransform(ToPxVec3(my_pos), ToPxQuat(my_transform->getRotation()));
	queryFilterData.flags = flags;

	result result;

	if (controller != Controllers::eReserved) {
		result.colision = EnginePhysics.Sweep(controller_geometry, my_pxtransform, ToPxVec3(dir), distance, hit_Buffer, queryFilterData);
	}
	else {
		result.colision = EnginePhysics.Sweep(my_geometry, my_pxtransform, ToPxVec3(dir), distance, hit_Buffer, queryFilterData);
	}
	result.hit = hit_Buffer;
	if (result.colision) {
		result.handle.fromVoidPtr(hit_Buffer.getAnyHit(0).actor->userData);
	}
	return result;
}
