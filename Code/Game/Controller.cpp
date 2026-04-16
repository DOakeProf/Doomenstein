#include "Game/Controller.hpp"

#include "Game/Map.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/Actor.hpp"
#include "Game/AI.hpp"

Controller::Controller(Map* map)
	: m_map(map)
{

}

void Controller::Possess(ActorHandle* handle)
{
	m_actorHandle = handle;
	m_map->GetActorByHandle(*handle)->m_controller = this;
}

void Controller::Depossess()
{
	if (m_actorHandle != nullptr)
	{
		Actor* actor = m_map->GetActorByHandle(*m_actorHandle);
		actor->m_controller = nullptr;
		if (actor->m_AIController != nullptr)
		{
			actor->m_AIController->Possess(m_actorHandle);
		}
		m_actorHandle = nullptr;
	}
}

Actor* Controller::GetActor()
{
	return m_map->GetActorByHandle(*m_actorHandle);
}

bool Controller::IsPlayer() const
{
	return false;
}

