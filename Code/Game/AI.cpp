#include "Game/AI.hpp"

#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Game/Game.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Timer.hpp"

AI::AI(Map* map, AIType aiType)
	: Controller(map)
	, m_type(aiType)
{
	Startup();
}

void AI::Startup()
{
	m_randomPatrolTimer = new Timer(2.f, m_map->m_game->m_gameClock);
	m_randomPatrolTimer->Start();
	ChooseNewPatrolDirection();
}

void AI::DamagedBy(ActorHandle* otherActor)
{
	m_targetActorHandle = otherActor;
}

void AI::Update()
{
	if (m_randomPatrolTimer->DecrementPeriodIfElapsed())
	{
		ChooseNewPatrolDirection();
		m_randomPatrolTimer->m_period = m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(1.5f, 6.f);
	}

	switch (m_type)
	{
		case AIType::MELEE: Update_Melee(); break;
		case AIType::RANGED: Update_Ranged(); break;
	}
}

void AI::Update_Melee()
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
				float angleOfSelfToOther2D = Atan2Degrees(selfToOtherActorNormalized.y, selfToOtherActorNormalized.x);
				float angleOfForwardVector2D = Atan2Degrees(forwardVector.y, forwardVector.x);
				float angleBetweenVectors2D = GetShortestAngularDispDegrees(angleOfSelfToOther2D, angleOfForwardVector2D);
				float maxTurnSpeedThisFrame = actor->m_definition->m_turnSpeed * (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
				actor->m_orientation.m_yawDegrees -= GetClamped(angleBetweenVectors2D, -maxTurnSpeedThisFrame, maxTurnSpeedThisFrame);
			}
		}
		else if (m_map == m_map->m_game->m_currentRiftMap) // Move into portal
		{
			MoveTowardNearestRift();
		}
		else // Move away from portal / Pathfind.
		{
			bool didMoveAway = MoveAwayFromRifts();

			if (!didMoveAway)
			{
				actor->MoveInDirection(m_randomPatrolDirection, actor->m_definition->m_walkSpeed);
				actor->TurnInDirection(m_randomPatrolDirection.GetOrientationAboutZDegrees(), actor->m_definition->m_turnSpeed);
			}
		}

		Vec3 forwardVector = actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp();
		RaycastResult3D result = m_map->RaycastWorldXY(actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight), forwardVector, actor->m_definition->m_radius + 0.5f);
		if (result.m_didImpact)
		{
			actor->Jump();
		}
	}
}

void AI::Update_Ranged()
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
				Vec3 selfToOtherActor = otherActor->m_position - actor->m_position;
				selfToOtherActor.z = 0.f; // Flatten the vector to the XY plane.
				Vec3 selfToOtherActorNormalized = selfToOtherActor.GetNormalized();

				float distToStop = 5.f;
				float distToStartSlowingDown = 7.f;
				float distBetweenSelfAndTarget = selfToOtherActor.GetLength();
				if (distBetweenSelfAndTarget < distToStop * 3.f) // attack when near player.
				{
					m_map->GetActorByHandle(*m_actorHandle)->Attack();
				}
				if (distBetweenSelfAndTarget < distToStartSlowingDown) // Slow down when near player.
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
				float angleOfSelfToOther2D = Atan2Degrees(selfToOtherActorNormalized.y, selfToOtherActorNormalized.x);
				float angleOfForwardVector2D = Atan2Degrees(forwardVector.y, forwardVector.x);
				float angleBetweenVectors2D = GetShortestAngularDispDegrees(angleOfSelfToOther2D, angleOfForwardVector2D);
				float maxTurnSpeedThisFrame = actor->m_definition->m_turnSpeed * (float)m_map->m_game->m_gameClock->GetDeltaSeconds();
				actor->m_orientation.m_yawDegrees -= GetClamped(angleBetweenVectors2D, -maxTurnSpeedThisFrame, maxTurnSpeedThisFrame);

				// Alter pitch for accurate ranged weapon usage
				Vec3 selfToOtherActorPitch = (otherActor->m_position + Vec3(0.f, 0.f, otherActor->m_definition->m_eyeHeight)) - (actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight));
				Vec3 selfToOtherActorPitchNormalized = selfToOtherActorPitch.GetNormalized();
				actor->m_orientation.m_pitchDegrees = -AsinDegrees(DotProduct3D(selfToOtherActorPitchNormalized, Vec3(0.f, 0.f, 1.f)));
			}
		}
		else if (m_map == m_map->m_game->m_currentRiftMap) // Move into portal
		{
			MoveTowardNearestRift();
		}
		else // Move away from portal / Pathfind.
		{
			bool didMoveAway = MoveAwayFromRifts();

			if (!didMoveAway)
			{
				actor->MoveInDirection(m_randomPatrolDirection, actor->m_definition->m_walkSpeed);
				actor->TurnInDirection(m_randomPatrolDirection.GetOrientationAboutZDegrees(), actor->m_definition->m_turnSpeed);
			}
		}

		Vec3 forwardVector = actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp();
		RaycastResult3D result = m_map->RaycastWorldXY(actor->m_position + Vec3(0.f, 0.f, actor->m_definition->m_eyeHeight), forwardVector, actor->m_definition->m_radius + 0.5f);
		if (result.m_didImpact)
		{
			actor->Jump();
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
				Vec3 selfToOther = otherActor->GetEyePos() - actor->GetEyePos();
				Vec3 forwardVector = actor->m_orientation.GetForwardDir_IFwd_JLeft_KUp();

				Vec3 selfToOtherNormalized = selfToOther.GetNormalized();
				float selfToOtherLength = selfToOther.GetLength();
				float angleOfSelfToOther2D = Atan2Degrees(selfToOtherNormalized.y, selfToOtherNormalized.x);
				float angleOfForwardVector2D = Atan2Degrees(forwardVector.y, forwardVector.x);
				float angleBetweenVectors2D = abs(GetShortestAngularDispDegrees(angleOfSelfToOther2D, angleOfForwardVector2D));

				if ((selfToOtherLength < actor->m_definition->m_sightRadius) &&
					(angleBetweenVectors2D < (actor->m_definition->m_sightAngle * 0.5f)))
				{
					RaycastResult3D resultXY = m_map->RaycastWorldXY(actor->GetEyePos(), selfToOtherNormalized, selfToOtherLength);
					RaycastResult3D resultZ = m_map->RaycastWorldZ(actor->GetEyePos(), selfToOtherNormalized, selfToOtherLength);

					if (!resultXY.m_didImpact && !resultZ.m_didImpact)
					{
						m_targetActorHandle = otherActor->m_handle;
					}
				}
			}
		}
	}
}

void AI::MoveTowardNearestRift()
{
	Actor* actor = GetActor();

	Rift* nearestRift = nullptr;
	float nearestRiftDist = 100.f;
	for (Rift* rift : s_rifts)
	{
		float distTowardRift = (rift->GetPosition() - GetActor()->m_position).GetLength();
		if (distTowardRift < nearestRiftDist)
		{
			nearestRift = rift;
			nearestRiftDist = distTowardRift;
		}
	}

	if (nearestRift != nullptr)
	{
		Vec3 actorToRift = (nearestRift->GetPosition() - actor->m_position);
		actorToRift.z = 0.f;
		actor->MoveInDirection(actorToRift.GetNormalized(), actor->m_definition->m_runSpeed);
		actor->TurnInDirection(actorToRift.GetOrientationAboutZDegrees(), actor->m_definition->m_turnSpeed);
	}
}

bool AI::MoveAwayFromRifts()
{
	Actor* actor = GetActor();

	Rift* nearestRift = nullptr;
	float nearestRiftDist = 100.f;
	for (Rift* rift : s_rifts)
	{
		float distTowardRift = (rift->GetPosition() - GetActor()->m_position).GetLength();
		if (distTowardRift < nearestRiftDist)
		{
			nearestRift = rift;
			nearestRiftDist = distTowardRift;
		}
	}

	if (nearestRift != nullptr)
	{
		Vec3 actorToRift = nearestRift->GetPosition() - actor->m_position;
		actorToRift.z = 0.f;
		if (actorToRift.GetLength() < 3.f)
		{
			actor->MoveInDirection(-actorToRift.GetNormalized(), actor->m_definition->m_runSpeed);
			actor->TurnInDirection(-actorToRift.GetOrientationAboutZDegrees(), actor->m_definition->m_turnSpeed);
			return true;
		}
	}

	return false;
}

void AI::ChooseNewPatrolDirection()
{
	float randomDirectionDegree = m_map->m_game->m_randomNumberGenerator->RollRandomFloatInRange(0.f, 360.f);
	Vec3 newRandomDirection = Vec3(1.f, 0.f, 0.f);
	newRandomDirection = newRandomDirection.GetRotatedAboutZDegrees(randomDirectionDegree);
	m_randomPatrolDirection = newRandomDirection;
}

void AI::Possess(ActorHandle* handle)
{
	m_actorHandle = handle;
	Actor* actor = m_map->GetActorByHandle(*handle);
	actor->m_controller = this;
	actor->m_AIController = this;
}

