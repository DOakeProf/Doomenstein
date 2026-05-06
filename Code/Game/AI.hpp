#pragma once

#include "Game/Controller.hpp"

enum class AIType
{
	NONE = -1,
	MELEE,
	RANGED,
	FLYING_MELEE,
	FLYING_RANGED,
	NUM_TYPES
};

class AI : public Controller
{
public:
	AI() = default;
	AI(Map* map, AIType aiType);
	~AI() = default;

	ActorHandle* m_targetActorHandle = nullptr;

	AIType m_type;

	void DamagedBy(ActorHandle* otherActor);
	void Update() override;
	void Update_Melee();
	void Update_Ranged();

	void Update_FindTargetActor();

	void Possess(ActorHandle* handle) override;
};