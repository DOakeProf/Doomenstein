#pragma once
#include <string>
#include <Vector>
#include <map>
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Mat44.hpp"

struct OBJ_Model;
struct OBJ_Face;
struct OBJ_Material;
struct OBJ_Object;
struct OBJ_Group;

struct Vec3;
struct Vec2;
struct Vertex;
struct AABB2;

class Texture;

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct OBJ_Model
{
public:
	OBJ_Model(std::string objFilePath, std::string matFilePath);
	~OBJ_Model();

	void AddVertsForModel(std::vector<Vertex>& verts, const Rgba8 color = Rgba8::WHITE, AABB2 UVs = AABB2::ZERO_TO_ONE);

	// Renders every object in the model.
	void Render(Mat44 modelToWorldTransform) const;
	// Renders a single specified object.
	void Render(std::string objectName, Mat44 modelToWorldTransform) const;

	void RenderToTexture(Mat44 modelToWorldTransform) const;
	void RenderToTexture(std::string objectName, Mat44 modelToWorldTransform) const;

private:
	std::vector<Vec3> m_vertexPosList;
	std::vector<Vec2> m_uvList;
	std::vector<Vec3> m_vertexNormalList;

	std::map< std::string, OBJ_Object*> m_objectMap;
	std::map< std::string, OBJ_Material*> m_materialMap;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct OBJ_Face
{
public:
	OBJ_Face(int v1, int v2, int v3,
		int uv1, int uv2, int uv3,
		int vn1, int vn2, int vn3);
	OBJ_Face() {}

	int v1;
	int v2;
	int v3;

	int vt1;
	int vt2;
	int vt3;

	int vn1;
	int vn2;
	int vn3;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct OBJ_Material
{
public:
	OBJ_Material() {}
	OBJ_Material(std::string TexturePath);

	Texture* m_texture = nullptr;
	Rgba8 m_diffuseColor;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct OBJ_Object
{
public:
	OBJ_Object() {}
	~OBJ_Object();
	
	std::vector<OBJ_Group*> m_groups;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct OBJ_Group
{
public:
	std::string				m_materialID;
	std::vector<Vertex>		m_verts;
	std::vector<OBJ_Face>	m_faces;
};