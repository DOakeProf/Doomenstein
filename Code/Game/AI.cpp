#include "Game/AI.hpp"

#include "Game/Map.hpp"
#include "Game/Actor.hpp"

#include "Engine/Math/MathUtils.hpp"

AI::AI(Map* map)
	: Controller(map)
{

}

void AI::DamagedBy(ActorHandle* otherActor)
{
	m_targetActorHandle = otherActor;
}

void AI::Update()
{
	// Detect good actors and target them
	if (m_targetActorHandle == nullptr)
	{
		Actor* actor = m_map->GetActorByHandle(*m_actorHandle);
		
		// Todo: implement the rest of the ai logic.
	}
}

void AI::Possess(ActorHandle* handle)
{
	m_actorHandle = handle;
	Actor* actor = m_map->GetActorByHandle(*handle);
	actor->m_controller = this;
	actor->m_AIController = this;
}

