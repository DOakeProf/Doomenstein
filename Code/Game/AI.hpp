#pragma once

#include "Game/Controller.hpp"

class AI : public Controller
{
public:
	AI() = default;
	AI(Map* map);
	~AI() = default;

	ActorHandle* m_targetActorHandle = nullptr;

	void DamagedBy(ActorHandle* otherActor);
	void Update();

	void Possess(ActorHandle* handle) override;
};