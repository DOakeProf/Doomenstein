#pragma once
#include "Game/Entity.hpp"

struct Camera;

class Portal : public Entity
{
public:
	Portal(Game* owner, Vec3 const& startingPosition);
	~Portal();

	virtual void Update() override;
	virtual void Render() const override;
	void RenderPortal() const;

	void Update_MoveCamera();

	void AssignPortal(Portal* otherPortal);
	Camera* GetCamera();
	Portal* GetOtherPortal();

	Vec3 bl;
	Vec3 br;
	Vec3 tr;
	Vec3 tl;

	bool m_isPlayerOnFrontSide = false;
	Camera* m_portalCamera = nullptr;

private:
	Portal* m_otherPortal = nullptr;

	
	std::vector<Vertex> m_frontVertexes;
	std::vector<Vertex> m_backVertexes;
	std::vector<Vertex> m_borderVertexes;
};