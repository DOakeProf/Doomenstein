#include "Game/Player.hpp"

#include "Game/Game.hpp"

#include "Engine/Renderer/Camera.hpp"

Player::Player(Game* owner, Vec3 const& startingPosition)
	: m_game(owner)
	, m_position(startingPosition)
{

}

void Player::Update()
{
	m_worldCamera->SetPositionAndOrientation(m_position, m_orientation);
}

void Player::Render() const
{

}

void Player::Die()
{
	m_isDead = true;
	m_isGarbage = true;
}

Mat44 Player::GetModelToWorldTransform() const
{
	Mat44 modelToWorld = Mat44();

	modelToWorld.AppendTranslation3D(m_position);

	Mat44 orientationMatrix = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	modelToWorld.Append(orientationMatrix);

	return modelToWorld;
}

Mat44 Player::GetWorldToModelTransform() const
{
	Mat44 ModelToWorld = GetModelToWorldTransform();

	return ModelToWorld.GetOrthonormalInverse();
}

bool Player::IsAlive() const
{
	return !m_isDead;
}
