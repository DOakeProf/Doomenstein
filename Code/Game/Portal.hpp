#pragma once
//#include "Game/Entity.hpp"

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Vertex.hpp"

struct Camera;
class Map;

class Portal
{
public:
	Portal(Map* map, Vec3 const& startingPosition);
	~Portal();

	void Update();
	void RenderOutline() const;
	void RenderPortal() const;

	void Update_MoveCamera();

	Mat44 GetModelToWorldTransform() const;
	Mat44 GetWorldToModelTransform() const;

	void AssignPortal(Portal* otherPortal);
	Camera* GetCamera() const;
	Portal* GetOtherPortal() const;
	EulerAngles GetOrientation() const;
	Vec3 GetPosition() const;

	void SetOrientation(EulerAngles const& newOrientation);

	Vec3 bl;
	Vec3 br;
	Vec3 tr;
	Vec3 tl;

	bool m_isPlayerOnFrontSide = false;
	Camera* m_portalCamera = nullptr;

private:
	Portal* m_otherPortal = nullptr;

	Map* m_map = nullptr;
	Vec3 m_position;
	EulerAngles m_orientation;
	
	std::vector<Vertex> m_frontVertexes;
	std::vector<Vertex> m_backVertexes;
	std::vector<Vertex> m_borderVertexes;
};