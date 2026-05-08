#pragma once

#include "Game/Controller.hpp"

#include "Engine/Math/Vec3.hpp"

class Timer;

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

	Vec3 m_randomPatrolDirection = Vec3(1.f, 0.f, 0.f);
	Timer* m_randomPatrolTimer;

	ActorHandle* m_targetActorHandle = nullptr;

	AIType m_type;

	void Startup();

	void DamagedBy(ActorHandle* otherActor);
	void Update() override;
	void Update_Melee();
	void Update_Ranged();

	void Update_FindTargetActor();
	void MoveTowardNearestRift();
	bool MoveAwayFromRifts();
	void ChooseNewPatrolDirection();

	void Possess(ActorHandle* handle) override;
};