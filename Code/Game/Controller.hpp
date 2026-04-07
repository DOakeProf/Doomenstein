#pragma once

struct ActorHandle;

class Map;
class Actor;

class Controller
{
public:
	Controller() = default;
	Controller(Map* map);
	~Controller() = default;

	virtual void Possess(ActorHandle* handle);
	Actor* GetActor();

	ActorHandle* m_actorHandle = nullptr;
	Map* m_map = nullptr;
};