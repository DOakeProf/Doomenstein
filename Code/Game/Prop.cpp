#include "Game/Prop.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/VertexUtils.hpp"

#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/objReader.hpp"
#include "Game/glTFReader.hpp"

Prop::Prop(Game* owner, Vec3 const& startingPosition)
	: Entity(owner, startingPosition)
{

}

void Prop::Update()
{

}

void Prop::Render() const
{
	// TODO: make an enum for render mode, have it list shape primitives and a model. If its model, render the model. if its a primitive, render that respective primitive?
	if (m_OBJModel != nullptr)
	{
		// If its empty just render every object.
		if (m_OBJObjectsToRender.empty())
		{
			m_OBJModel->Render(GetModelToWorldTransform());
		}
		// If it has object names render all objects listed.
		else 
		{
			for (int objectIndex = 0; objectIndex < m_OBJObjectsToRender.size(); ++objectIndex)
			{
				std::string currentObjectName = m_OBJObjectsToRender[objectIndex];
				m_OBJModel->Render(currentObjectName, GetModelToWorldTransform());
			}
		}
	}
	else if (m_GLTFModel != nullptr)
	{
		g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), m_color);
		m_GLTFModel->Test_RenderModel();
	}
	else if (m_isIndexed)
	{
		g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
		if (!m_dontBindTexture)
		{
			g_engine->m_render->BindTexture(m_texture);
		}

		g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), m_color);
		g_engine->m_render->DrawIndexedVertexList(&m_vertexes, &m_indexes);
	}
	else
	{
		g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
		if (!m_dontBindTexture)
		{
			g_engine->m_render->BindTexture(m_texture);
		}

		g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), m_color);
		g_engine->m_render->DrawVertexList(&m_vertexes);
	}
}

void Prop::RenderToTexture() const
{
	// TODO: make an enum for render mode, have it list shape primitives and a model. If its model, render the model. if its a primitive, render that respective primitive.
	if (m_OBJModel != nullptr)
	{
		// If its empty just render every object.
		if (m_OBJObjectsToRender.empty())
		{
			m_OBJModel->RenderToTexture(GetModelToWorldTransform());
		}
		// If it has object names render all objects listed.
		else
		{
			for (int objectIndex = 0; objectIndex < m_OBJObjectsToRender.size(); ++objectIndex)
			{
				std::string currentObjectName = m_OBJObjectsToRender[objectIndex];
				m_OBJModel->RenderToTexture(currentObjectName, GetModelToWorldTransform());
			}
		}
	}
	else
	{
		g_engine->m_render->SetBlendMode(BlendMode::ALPHA);
		g_engine->m_render->BindTexture(m_texture);

		g_engine->m_render->SetModelConstants(GetModelToWorldTransform(), m_color);
		//g_engine->m_render->RenderToTexture(&m_vertexes);
	}
}

void Prop::AddObjectToRender(std::string objectName)
{
	m_OBJObjectsToRender.push_back(objectName);
}

Mat44 Prop::GetModelToWorldTransform() const
{
	Mat44 modelToWorld = Mat44();

	modelToWorld.AppendTranslation3D(m_position);

	if (m_isLookingAtOther)
	{
		Mat44 lookAtMatrix = GetLookAtMatrix(m_game->m_player->m_position, m_game->m_player->GetModelToWorldTransform().GetKBasis3D());
		modelToWorld.Append(lookAtMatrix);
	}
	else
	{
		Mat44 orientationMatrix = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
		modelToWorld.Append(orientationMatrix);
	}

	return modelToWorld;
}

Mat44 Prop::GetLookAtMatrix(Vec3 entityToLookAtPosition, Vec3 entityToLookAtUpVector) const
{
	Vec3 thisToEntityToLookAt = m_position - entityToLookAtPosition;
	Vec3 forwardVector = thisToEntityToLookAt.GetNormalized();
	Vec3 rightVector = CrossProduct3D(entityToLookAtUpVector, forwardVector);
	Vec3 upVector = CrossProduct3D(forwardVector, rightVector);
	Mat44 lookAtMatrix = Mat44(forwardVector,rightVector,upVector, Vec3(0.f,0.f,0.f));
	return lookAtMatrix;
}