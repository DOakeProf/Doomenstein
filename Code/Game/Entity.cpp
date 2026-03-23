#include "Game/Entity.hpp"
#include "GameCommon.hpp"
#include "Game/Game.hpp"

#include "Engine/Core/Clock.hpp"

Entity::Entity(Game* owner, Vec3 const& startingPosition)
	: m_game(owner)
	, m_position(startingPosition)
{
}

bool Entity::IsAlive() const
{
	return !m_isDead;
}

void Entity::Die() 
{
	m_isDead = true;
	m_isGarbage = true;
}

void Entity::Update()
{
	float deltaSeconds = (float)m_game->m_gameClock->GetDeltaSeconds();

	m_position += m_velocity * deltaSeconds;
	m_orientation += m_angularVelocity * deltaSeconds;
}

Mat44 Entity::GetModelToWorldTransform() const
{
	Mat44 modelToWorld = Mat44();

	modelToWorld.AppendTranslation3D(m_position);

	Mat44 orientationMatrix = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	modelToWorld.Append(orientationMatrix);

	return modelToWorld;
}

Mat44 Entity::GetWorldToModelTransform() const
{
	Mat44 ModelToWorld = GetModelToWorldTransform();

	return ModelToWorld.GetOrthonormalInverse();
}
