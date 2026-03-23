#pragma once
#include "Game/Entity.hpp"

struct Camera;

class Player : public Entity
{
public:
	Player(Game* owner, Vec3 const& startingPosition);
	virtual ~Player() = default;

	virtual void Update();
	virtual void Render() const override;

	Camera* m_worldCamera = nullptr;
	Camera* m_screenCamera = nullptr;
};