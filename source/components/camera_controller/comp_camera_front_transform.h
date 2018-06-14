#pragma once

#include "components/comp_base.h"

class TCompCameraFrontTransform : public TCompBase {
	DECL_SIBLING_ACCESS();

public:
	void debugInMenu();
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);

private:
	std::string target_transorm_name;
	CEntity * target_transform;
	CEntity * player_transform;
};

