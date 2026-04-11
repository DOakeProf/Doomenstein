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

	virtual void Update() = 0;

	virtual void Possess(ActorHandle* handle);
	virtual void Depossess();
	Actor* GetActor();

	virtual bool IsPlayer() const;

	ActorHandle* m_actorHandle = nullptr;
	Map* m_map = nullptr;
};