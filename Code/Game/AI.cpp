#include "Game/AI.hpp"

#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Game/Game.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Clock.hpp"

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
	Actor* actor = m_map->GetActorByHandle(*m_actorHandle);
	if (actor != nullptr && !actor->m_isDead)
	{
		Update_FindTargetActor();

		if (m_targetActorHandle != nullptr)
		{
			Actor* otherActor = m_map->GetActorByHandle(*m_targetActorHandle);
			if (otherActor == nullptr)
			{
				m_targetActorHandle = nullptr;
			}
			else
			{
				// Move towards target.
				float deltaSeconds = m_map->m_game->m_gameClock->GetDeltaSeconds();
				Vec3 selfToOtherActor = otherActor->m_position - actor->m_position;
				selfToOtherActor.z = 0.f; // Flatten the vector to the XY plane.
				Vec3 selfToOtherActorNormalized = selfToOtherActor.GetNormalized();

				float distToStop = 0.7f;
				float distToStartSlowingDown = 2.f;
				float distBetweenSelfAndTarget = selfToOtherActor.GetLength();
				if (distBetweenSelfAndTarget < distToStop) // Stop and attack when near player.
				{
					m_map->GetActorByHandle(*m_actorHandle)->Attack();
				}
				else if (distBetweenSelfAndTarget < distToStartSlowingDown) // Slow down when near player.
				{
					actor->MoveInDirection(selfToOtherActorNormalized, actor->m_definition->m_runSpeed * 
					(distBetweenSelfAndTarget - distToStop) / (distToStartSlowingDown - distToStop));
					m_map->GetActorByHandle(*m_actorHandle)->Attack();
				}
				else
				{
					actor->MoveInDirection(selfToOtherActorNormalized, actor->m_definition->m_runSpeed);
				}

				// Orient towards target.
				Vec3 forwardVector = actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp();
				float selfToOtherLength = selfToOtherActor.GetLength();
				float angleOfSelfToOther2D = Atan2Degrees(selfToOtherActorNormalized.y, selfToOtherActorNormalized.x);
				float angleOfForwardVector2D = Atan2Degrees(forwardVector.y, forwardVector.x);
				float angleBetweenVectors2D = GetShortestAngularDispDegrees(angleOfSelfToOther2D, angleOfForwardVector2D);
				float maxTurnSpeedThisFrame = actor->m_definition->m_turnSpeed * m_map->m_game->m_gameClock->GetDeltaSeconds();
				actor->m_orientation.m_yawDegrees -= GetClamped(angleBetweenVectors2D, -maxTurnSpeedThisFrame, maxTurnSpeedThisFrame);
			}
		}
	}
}

void AI::Update_FindTargetActor()
{
	// Detect good actors and target them
	if (m_targetActorHandle == nullptr)
	{
		Actor* actor = m_map->GetActorByHandle(*m_actorHandle);

		// Todo: implement the rest of the ai logic.
		std::vector<Actor*> actors = m_map->GetActors();
		for (int actorIndex = 0; actorIndex < actors.size(); ++actorIndex)
		{
			Actor* otherActor = actors[actorIndex];
			if (otherActor != nullptr && otherActor->m_definition->m_faction == "Marine")
			{
				Vec3 selfToOther = otherActor->m_position - actor->m_position;
				Vec3 forwardVector = actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp();

				Vec3 selfToOtherNormalized = selfToOther.GetNormalized();
				float selfToOtherLength = selfToOther.GetLength();
				float angleOfSelfToOther2D = Atan2Degrees(selfToOtherNormalized.y, selfToOtherNormalized.x);
				float angleOfForwardVector2D = Atan2Degrees(forwardVector.y, forwardVector.x);
				float angleBetweenVectors2D = abs(GetShortestAngularDispDegrees(angleOfSelfToOther2D, angleOfForwardVector2D));

				if ((selfToOtherLength < actor->m_definition->m_sightRadius) &&
					(angleBetweenVectors2D < (actor->m_definition->m_sightAngle * 0.5f)))
				{
					RaycastResult3D resultXY = m_map->RaycastWorldXY(actor->m_position, selfToOtherNormalized, selfToOtherLength);
					RaycastResult3D resultZ = m_map->RaycastWorldZ(actor->m_position, selfToOtherNormalized, selfToOtherLength);

					if (!resultXY.m_didImpact && !resultZ.m_didImpact)
					{
						m_targetActorHandle = otherActor->m_handle;
					}
				}
			}
		}
	}
}

void AI::Possess(ActorHandle* handle)
{
	m_actorHandle = handle;
	Actor* actor = m_map->GetActorByHandle(*handle);
	actor->m_controller = this;
	actor->m_AIController = this;
}

