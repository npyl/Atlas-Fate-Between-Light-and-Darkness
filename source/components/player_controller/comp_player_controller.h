#pragma once

#include "components/comp_base.h"
#include "geometry/transform.h"
#include "components/ia/ai_controller.h"
#include "entity/common_msgs.h"

class TCompPlayerController : public IAIController {

	std::map<std::string, CRenderMesh*> mesh_states;
	physx::PxQueryFilterData shadowMergeFilter;
	physx::PxQueryFilterData playerFilter;
	/* Camera stack, to bypass entity delayed loading */
	/* Replace everything here with a real camera stack */
	std::string camera_shadowmerge_hor;
	std::string camera_shadowmerge_ver;
	std::string camera_thirdperson;
	std::string camera_shadowmerge_aux;
	std::string camera_actual;

	/* Player stamina */
	float stamina;
	float maxStamina;
	float minStamina;
	float minStaminaToMerge;
	float dcrStaminaOnPlaceMultiplier;
	float dcrStaminaGround;
	float dcrStaminaWall;
	float incrStamina = 20.f;

	float dcrStaminaGroundAux = 0.f;
	float dcrStaminaWallAux = 0.f;
	
	/* Player speeds*/
	float runSpeedFactor;
	float walkSpeedFactor;
	float walkSlowSpeedFactor;
	float walkCrouchSpeedFactor;
	float walkSlowCrouchSpeedFactor;
	float currentSpeed;
	float rotationSpeed;
	float distToAttack;
	float distToSM;
	float canAttack = false;
	CHandle enemyToAttack;
	CHandle enemyToSM;

	bool crouched = false;

	/* Aux offset */
	float maxGroundDistance = 0.3f;
	float convexMaxDistance = 0.55f;

	/* Timers */
	int timesToPressRemoveInhibitorKey;
	int timesRemoveInhibitorKeyPressed = 0;
	float timerForPressingRemoveInhibitorKey = 0.f;
	float maxTimeToSMFalling;
	float timeFallingWhenSMPressed = 0.f;
	float timeFalling = 0.f;
	float timeFallingToDie;

	std::string target_name;
	bool inhibited = false;
	void onMsgPlayerHit(const TMsgPlayerHit& msg);
	void onMsgPlayerShotInhibitor(const TMsgInhibitorShot& msg);
	void onMsgPlayerIlluminated(const TMsgPlayerIlluminated& msg);
	void onMsgPlayerKilled(const TMsgPlayerDead& msg);

	/* Aux variables */
	std::string auxStateName = "";
	
	/* Private aux functions */

	void movePlayer(const float dt);
	void movePlayerShadow(const float dt);
	const bool motionButtonsPressed();

	bool manageInhibition(float dt);

	void allowAttack(bool allow, CHandle enemy);
	CHandle checkTouchingStunnedEnemy();
	bool checkEnemyInShadows(CHandle enemy);
	void manageCrouch();
	bool playerInFloor();
	bool canStandUp();
	void setPlayerDead();
	void ResetPlayer(void);

	const bool ConcaveTest(void);
	const bool ConvexTest(void);
	const bool ShadowTest(void);
	const bool GroundTest(void);

	DECL_SIBLING_ACCESS();

public:

	//VEC3 delta_movement;

	void debugInMenu();
	void renderDebug();
	void load(const json& j, TEntityParseContext& ctx);
	void Init();

	static void registerMsgs();

	/* States */
	void IdleState(float);
	void MotionState(float);	//Movement
	void PushState(float);
	void AttackState(float);
	void ProbeState(float);
	void RemovingInhibitorState(float);
	void ShadowMergingEnterState(float);
	void ShadowMergingHorizontalState(float);
	void ShadowMergingVerticalState(float);
	void ShadowMergingEnemyState(float);
	void ShadowMergingLandingState(float);
	void ShadowMergingExitState(float);
	void FallingState(float);
	void LandingState(float);
	void HitState(float);
	void DeadState(float);

	//bool checkShadows();
	const bool isInShadows();
	const bool isDead();
	const bool checkAttack();
	const bool isInhibited() { return inhibited; };
	const bool checkPaused();
};