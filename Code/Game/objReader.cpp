#include "Game/objReader.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/Engine.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"

OBJ_Model::OBJ_Model(std::string objFilePath, std::string matFilePath)
{
	std::string matContents;
	FileReadToString(matContents, matFilePath);

	Strings matLines = SplitStringOnDelimiter(matContents, '\n');
	Strings filePathSplit = SplitStringOnDelimiter(matFilePath, '/');
	std::string objDirectory = "";
	for (int filePathIndex = 0; filePathIndex < filePathSplit.size() - 1; ++filePathIndex)
	{
		objDirectory += filePathSplit[filePathIndex] + '/';
	}


	OBJ_Material* currentMaterial = nullptr;
	for (int lineIndex = 0; lineIndex < matLines.size(); ++lineIndex)
	{
		matLines[lineIndex].erase(std::remove(matLines[lineIndex].begin(), matLines[lineIndex].end(), '\r'), matLines[lineIndex].end());
		Strings lineParts = SplitStringOnDelimiter(matLines[lineIndex], ' ');

		if (lineParts[0] == "newmtl")
		{
			m_materialMap[lineParts[1]] = new OBJ_Material();
			currentMaterial = m_materialMap[lineParts[1]];
		}
		else if (lineParts[0] == "map_Kd")
		{
			std::string textureFilePathString = objDirectory + lineParts[1];
			//textureFilePathString.erase(std::remove(textureFilePathString.begin(), textureFilePathString.end(), '\r'), textureFilePathString.end());
			const char* textureFilePath = textureFilePathString.data();
			currentMaterial->m_texture = g_engine->m_render->CreateOrGetTextureFromFile(textureFilePath);
		}
		else if (lineParts[0] == "Kd")
		{
			unsigned char rColor = DenormalizeByte((unsigned char)atoi(lineParts[1].c_str()));
			unsigned char gColor = DenormalizeByte((unsigned char)atoi(lineParts[2].c_str()));
			unsigned char bColor = DenormalizeByte((unsigned char)atoi(lineParts[3].c_str()));
			currentMaterial->m_diffuseColor = Rgba8(rColor, gColor, bColor);
		}
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	std::string objContents;
	FileReadToString(objContents, objFilePath);

	Strings objLines = SplitStringOnDelimiter(objContents, '\n');

	int vertexPosIndex = 0;
	int uvPosIndex = 0;


	OBJ_Object* currentOBJ_Object = nullptr;
	OBJ_Group* currentOBJ_Group = nullptr;
	std::string currentMaterialID;
	for (int lineIndex = 0; lineIndex < objLines.size(); ++lineIndex)
	{
		objLines[lineIndex].erase(std::remove(objLines[lineIndex].begin(), objLines[lineIndex].end(), '\r'), objLines[lineIndex].end());
		Strings lineParts = SplitStringOnDelimiter(objLines[lineIndex], ' ');
		if (lineParts[0] == "v") // Line is a vertex pos
		{
			m_vertexPosList.push_back(Vec3(std::stof(lineParts[1]),
												   std::stof(lineParts[2]),
												   std::stof(lineParts[3])));
			++vertexPosIndex;
		}
		else if (lineParts[0] == "vt") // Line is a UV pos
		{
			m_uvList.push_back(Vec2(std::stof(lineParts[1]),
									std::stof(lineParts[2])));
			++uvPosIndex;
		}
		else if (lineParts[0] == "vn") // Line is a UV pos
		{
			m_vertexNormalList.push_back(Vec3(std::stof(lineParts[1]),
											  std::stof(lineParts[2]),
											  std::stof(lineParts[3])));
			++uvPosIndex;
		}
		else if (lineParts[0] == "o")
		{
			m_objectMap[lineParts[1]] = new OBJ_Object();
			OBJ_Object* previousOBJ_Object = currentOBJ_Object;
			currentOBJ_Object = m_objectMap[lineParts[1]];
			if (currentOBJ_Group != nullptr && previousOBJ_Object == nullptr)
			{
				currentOBJ_Object->m_groups.push_back(currentOBJ_Group);
			}
		}
		else if (lineParts[0] == "g")
		{
			OBJ_Group* newGroup = new OBJ_Group();
			currentOBJ_Group = newGroup;
			if (currentOBJ_Object != nullptr)
			{
				currentOBJ_Object->m_groups.push_back(newGroup);
			}
			if (!currentMaterialID.empty())
			{
				currentOBJ_Group->m_materialID = currentMaterialID;
			}
		}
		else if (lineParts[0] == "usemtl")
		{
			currentMaterialID = lineParts[1];
			if (currentOBJ_Group != nullptr)
			{
				// If we have a group with no material, assign that group to this material.
				if (currentOBJ_Group->m_materialID.empty())
				{
					currentOBJ_Group->m_materialID = currentMaterialID;
				}
				// If we have a group WITH a seperate material, make another group.
				else if (currentOBJ_Group->m_materialID != currentMaterialID)
				{
					OBJ_Group* newGroup = new OBJ_Group();
					currentOBJ_Group = newGroup;
					if (currentOBJ_Object != nullptr)
					{
						currentOBJ_Object->m_groups.push_back(newGroup);
					}
					currentOBJ_Group->m_materialID = currentMaterialID;
				}
			}
		}
		else if (lineParts[0] == "f") // Line is a face
		{
			// TODO: Handle the case where the OBJ doesnt specify groups or objects, but instead only specifies material.
			// If an object/group doesnt exist, generate one.
			if (currentOBJ_Object == nullptr)
				//m_currentOBJ_Object != m_objectMap[m_currentMaterialID] // Assuming the previous object was generated, 
				// is our current material ID the same as our 
				// generated one's ID?
			{
				m_objectMap[currentMaterialID] = new OBJ_Object();
				currentOBJ_Object = m_objectMap[currentMaterialID];
				if (currentOBJ_Group != nullptr)
				{
					currentOBJ_Object->m_groups.push_back(currentOBJ_Group);
				}
			}
			if (currentOBJ_Group == nullptr)
			{
				OBJ_Group* newGroup = new OBJ_Group();
				currentOBJ_Group = newGroup;
				currentOBJ_Object->m_groups.push_back(newGroup);
				currentOBJ_Group->m_materialID = currentMaterialID;
			}

			Strings vert1 = SplitStringOnDelimiter(lineParts[1], '/');
			Strings vert2 = SplitStringOnDelimiter(lineParts[2], '/');
			Strings vert3 = SplitStringOnDelimiter(lineParts[3], '/');

			Vec3 pos1 = m_vertexPosList[std::stoi(vert1[0]) - 1];
			Vec3 pos2 = m_vertexPosList[std::stoi(vert2[0]) - 1];
			Vec3 pos3 = m_vertexPosList[std::stoi(vert3[0]) - 1];
			Vec2 uv1 = m_uvList[std::stoi(vert1[1]) - 1];
			Vec2 uv2 = m_uvList[std::stoi(vert2[1]) - 1];
			Vec2 uv3 = m_uvList[std::stoi(vert3[1]) - 1];
			Vec3 normal1 = m_vertexNormalList[std::stoi(vert1[2]) - 1];
			Vec3 normal2 = m_vertexNormalList[std::stoi(vert2[2]) - 1];
			Vec3 normal3 = m_vertexNormalList[std::stoi(vert3[2]) - 1];

			currentOBJ_Group->m_verts.emplace_back(pos1, Rgba8::WHITE, uv1);
			currentOBJ_Group->m_verts.emplace_back(pos2, Rgba8::WHITE, uv2);
			currentOBJ_Group->m_verts.emplace_back(pos3, Rgba8::WHITE, uv3);
		}
	}
}

OBJ_Model::~OBJ_Model()
{
	for (std::map< std::string, OBJ_Object* >::const_iterator it = m_objectMap.begin(); it != m_objectMap.end(); ++it)
	{
		OBJ_Object* currentObject = it->second;
		delete currentObject;
		m_objectMap[it->first] = nullptr;
	}
	for (std::map< std::string, OBJ_Material* >::const_iterator it = m_materialMap.begin(); it != m_materialMap.end(); ++it)
	{
		OBJ_Material* currentObject = it->second;
		delete currentObject;
		m_materialMap[it->first] = nullptr;
	}
}

void OBJ_Model::Render(Mat44 modelToWorldTransform) const
{
	for (std::map< std::string, OBJ_Object* >::const_iterator it = m_objectMap.begin(); it != m_objectMap.end(); ++it)
	{
		OBJ_Object* currentObject = it->second;
		for (int groupIndex = 0; groupIndex < currentObject->m_groups.size(); ++groupIndex)
		{
			OBJ_Group* currentGroup = currentObject->m_groups[groupIndex];
			std::string testString = currentGroup->m_materialID;
			OBJ_Material* currentMaterial = (m_materialMap.at(currentGroup->m_materialID));
			g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
			/*g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);*/
			g_engine->m_render->BindTexture(currentMaterial->m_texture);

			g_engine->m_render->SetModelConstants(modelToWorldTransform, currentMaterial->m_diffuseColor);
			g_engine->m_render->DrawVertexList(&currentGroup->m_verts);
		}
	}
}

void OBJ_Model::Render(std::string objectName, Mat44 modelToWorldTransform) const
{
	OBJ_Object* currentObject = m_objectMap.at(objectName);
	for (int groupIndex = 0; groupIndex < currentObject->m_groups.size(); ++groupIndex)
	{
		OBJ_Group* currentGroup = currentObject->m_groups[groupIndex];
		OBJ_Material* currentMaterial = m_materialMap.at(currentGroup->m_materialID);
		//g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
		/*g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);*/
		g_engine->m_render->BindTexture(currentMaterial->m_texture);

		g_engine->m_render->SetModelConstants(modelToWorldTransform, currentMaterial->m_diffuseColor);
		g_engine->m_render->DrawVertexList(&currentGroup->m_verts);
	}
}

void OBJ_Model::RenderToTexture(Mat44 modelToWorldTransform) const
{
	for (std::map< std::string, OBJ_Object* >::const_iterator it = m_objectMap.begin(); it != m_objectMap.end(); ++it)
	{
		OBJ_Object* currentObject = it->second;
		for (int groupIndex = 0; groupIndex < currentObject->m_groups.size(); ++groupIndex)
		{
			OBJ_Group* currentGroup = currentObject->m_groups[groupIndex];
			std::string testString = currentGroup->m_materialID;
			OBJ_Material* currentMaterial = (m_materialMap.at(currentGroup->m_materialID));
			g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
			/*g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);*/
			g_engine->m_render->BindTexture(currentMaterial->m_texture);

			g_engine->m_render->SetModelConstants(modelToWorldTransform, currentMaterial->m_diffuseColor);
			//g_engine->m_render->RenderToTexture(&currentGroup->m_verts);
		}
	}
}

void OBJ_Model::RenderToTexture(std::string objectName, Mat44 modelToWorldTransform) const
{
	OBJ_Object* currentObject = m_objectMap.at(objectName);
	for (int groupIndex = 0; groupIndex < currentObject->m_groups.size(); ++groupIndex)
	{
		OBJ_Group* currentGroup = currentObject->m_groups[groupIndex];
		OBJ_Material* currentMaterial = m_materialMap.at(currentGroup->m_materialID);
		//g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
		/*g_engine->m_render->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);*/
		g_engine->m_render->BindTexture(currentMaterial->m_texture);

		g_engine->m_render->SetModelConstants(modelToWorldTransform, currentMaterial->m_diffuseColor);
		//g_engine->m_render->RenderToTexture(&currentGroup->m_verts);
	}
}

OBJ_Face::OBJ_Face(int v1, int v2, int v3, int uv1, int uv2, int uv3, int vn1, int vn2, int vn3)
	: v1(v1)
	, v2(v2)
	, v3(v3)
	, vt1(uv1)
	, vt2(uv2)
	, vt3(uv3)
	, vn1(vn1)
	, vn2(vn2)
	, vn3(vn3)
{

}

OBJ_Material::OBJ_Material(std::string TexturePath)
{
	m_texture = g_engine->m_render->CreateOrGetTextureFromFile(TexturePath.c_str());
}

OBJ_Object::~OBJ_Object()
{
	for (int groupIndex = 0; groupIndex < m_groups.size(); ++groupIndex)
	{
		OBJ_Group* currentGroup = m_groups[groupIndex];
		delete currentGroup;
		m_groups[groupIndex] = nullptr;
	}
}
