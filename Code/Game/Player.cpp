#include "Game/Player.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"

Player::Player(Game* owner, Vec3 const& startingPosition)
	: Entity(owner, startingPosition)
{

}

void Player::Update()
{
	m_worldCamera->SetPositionAndOrientation(m_position, m_orientation);
}

void Player::Render() const
{

}
