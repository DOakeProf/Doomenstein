#include "Game/Controller.hpp"

#include "Game/Map.hpp"
#include "Game/ActorHandle.hpp"

Controller::Controller(Map* map)
	: m_map(map)
{

}

void Controller::Possess(ActorHandle* handle)
{
	m_actorHandle = handle;
	m_map->GetActorByHandle(*handle)->m_controller = this;
}

Actor* Controller::GetActor()
{
	return m_map->GetActorByHandle(*m_actorHandle);
}

