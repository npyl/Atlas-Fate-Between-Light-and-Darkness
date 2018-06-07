#ifndef INC_COMP_SKELETON_H_
#define INC_COMP_SKELETON_H_

#include "../comp_base.h"
#include "render/cte_buffer.h"
#include "ctes.h"

class CalModel;

struct TCompSkeleton : public TCompBase {

	std::map<std::string, int> stringAnimationIdMap;
	std::vector<int> feetBonesId;
	int actualCycleAnimId[2];
	float cyclicAnimationWeight;
	float lastFrameCyclicAnimationWeight;
	CRenderCte<CCteSkinBones> cb_bones;
	CalModel* model = nullptr;
	bool rootMovementAction = false;
	bool rootMovementCyclic = false;
	Vector3 lastFrameRootPos;

	TCompSkeleton();
	~TCompSkeleton();
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	void updateCtesBones();

	void changeCyclicAnimation(int anim1Id, float speed = 1.0f, int anim2Id = -1, float weight = 1.0f, bool rootMoved = false, float in_delay = 0.15f, float out_delay = 0.15f);
	void executeActionAnimation(int animId, float speed = 1.0f, bool rootMoved = false, float in_delay = 0.15f, float out_delay = 0.15f);

	void setCyclicAnimationWeight(float new_value);
	float getCyclicAnimationWeight();
	int getAnimationIdByName(std::string animName);
	bool actionAnimationOnExecution();
	bool isExecutingCyclicAnimation(int animId);
	bool isExecutingActionAnimation(std::string animName);
	float getAnimationDuration(int animId);
	void guessFeetBonesId(int feetNum);
	void setFeetId(std::vector<int> feetId);
	std::vector<VEC3> getFeetPositions();
	Vector3 getBonePosById(int bone_id);

	DECL_SIBLING_ACCESS();
};

#endif
