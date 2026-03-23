#pragma once
#include "Game/Entity.hpp"
#include "Engine/Math/Mat44.hpp"

#include <string>

class Texture;
class glTF_Asset;

struct OBJ_Model;

class Prop : public Entity
{
public:
	std::vector<Vertex>			m_vertexes;
	std::vector<unsigned int>	m_indexes;
	Rgba8						m_color = Rgba8::WHITE;
	Texture*					m_texture = nullptr;
	bool						m_isLookingAtOther = false;
	bool						m_dontBindTexture = false;
	bool						m_isIndexed = false;

	OBJ_Model*					m_OBJModel;
	std::vector<std::string>	m_OBJObjectsToRender;

	glTF_Asset*					m_GLTFModel = nullptr;

public:
	Prop(Game* owner, Vec3 const& startingPosition);
	virtual ~Prop() = default;
	virtual void Update();
	virtual void Render() const override;
	void RenderToTexture() const;

	void AddObjectToRender(std::string objectName);

	virtual Mat44 GetModelToWorldTransform() const override;
	 Mat44 GetLookAtMatrix(Vec3 entityToLookAtPosition, Vec3 entityToLookAtUpVector) const;
};