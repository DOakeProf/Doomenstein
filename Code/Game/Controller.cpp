#include "Game/Controller.hpp"

#include "Game/Map.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/Actor.hpp"

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
		m_map->GetActorByHandle(*m_actorHandle)->m_controller = nullptr;
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

