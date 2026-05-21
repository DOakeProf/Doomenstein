#include "Game/glTFReader.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/Engine.hpp"

glTF_Asset::glTF_Asset(const char* gltfFilePath, const char* gltfBinPath)
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Get the shader which includes the node transform constant buffer
	//m_glTFAnimatedShader = g_engine->m_render->CreateShader("Data/Shaders/glTFAnimated", VertexType::VERTEX_PCU);

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Read the json data
	std::string glTFContents;
	FileReadToString(glTFContents, gltfFilePath);
	JsonData jsonData = ReadJsonFromFile(gltfFilePath);
	Strings filePathSplit = SplitStringOnDelimiter(std::string(gltfFilePath), '/');
	m_path = "";
	for (int fileSplitIndex = 0; fileSplitIndex < filePathSplit.size() - 1; ++fileSplitIndex)
	{
		m_path += filePathSplit[fileSplitIndex] + "/";
	}

	Initialize_LoadAllBuffers(jsonData);
	Initialize_LoadAllBufferViews(jsonData);
	Initialize_LoadAllAccessors(jsonData);
	Initialize_LoadAllMaterials(jsonData);
	Initialize_LoadAllMeshes(jsonData);
	Initialize_LoadAllNodes(jsonData);
	Initialize_LoadAllScenes(jsonData);

	Initialize_LoadAllMaterials(jsonData);
	Initialize_LoadAllTextures(jsonData);
	Initialize_LoadAllImages(jsonData);
	Initialize_LoadAllSamplers(jsonData);

	FileReadToBuffer(m_buffers[0]->m_data, gltfBinPath); // TODO: Make this support multiple buffers

	Initialize_LoadBufferInformationToMeshes();
	//Initialize_LoadBufferInformationForAnimations();
}

glTF_Asset::~glTF_Asset()
{

}

void glTF_Asset::Initialize_LoadAllScenes(JsonData const& jsonData)
{
	JsonData scenes = GetValueFromJsonData(jsonData, "scenes");
	JsonArray scenesArray = GetArrayFromJsonData(scenes);
	for (int arrayIndex = 0; arrayIndex < scenesArray.size(); ++arrayIndex)
	{
		glTF_Scene* newObject = new glTF_Scene();
		JsonData currentData = scenesArray[arrayIndex];

		newObject->m_name = GetValueFromJsonData(currentData, "name", "");
		JsonArray nodeIndexArray = GetArrayFromJsonData(GetValueFromJsonData(currentData, "nodes"));
		for (int nodeIndexIndex = 0; nodeIndexIndex < nodeIndexArray.size(); ++nodeIndexIndex)
		{
			int nodeIndex = GetValueFromJsonData(nodeIndexArray[nodeIndexIndex], -1);
			newObject->m_nodeIndexes.push_back(nodeIndex);
		}

		m_scenes.push_back(newObject);
	}
}

void glTF_Asset::Initialize_LoadAllNodes(JsonData  const& jsonData)
{
	JsonData nodes = GetValueFromJsonData(jsonData, "nodes");
	JsonArray nodesArray = GetArrayFromJsonData(nodes);
	for (int arrayIndex = 0; arrayIndex < nodesArray.size(); ++arrayIndex)
	{
		glTF_Node* newObject = new glTF_Node();
		JsonData currentData = nodesArray[arrayIndex];

		newObject->m_owner = this;
		newObject->m_name = GetValueFromJsonData(currentData, "name", "");
		newObject->m_meshIndex = GetValueFromJsonData(currentData, "mesh", -1);
		if (DoesValueExist(currentData, "children"))
		{
			JsonArray childIndexArray = GetArrayFromJsonData(GetValueFromJsonData(currentData, "children"));
			for (int childIndexIndex = 0; childIndexIndex < childIndexArray.size(); ++childIndexIndex)
			{
				int nodeIndex = GetValueFromJsonData(childIndexArray[childIndexIndex], -1);
				newObject->m_childIndexes.push_back(nodeIndex);
			}
		}
		if (DoesValueExist(currentData, "matrix"))
		{
			JsonArray matrixArray = GetArrayFromJsonData(GetValueFromJsonData(currentData, "matrix"));
			Vec4 iBasis = Vec4
			(
				(float)GetValueFromJsonData(matrixArray[0], -1),
				(float)GetValueFromJsonData(matrixArray[1], -1),
				(float)GetValueFromJsonData(matrixArray[2], -1),
				(float)GetValueFromJsonData(matrixArray[3], -1)
			);
			Vec4 jBasis = Vec4
			(
				(float)GetValueFromJsonData(matrixArray[4], -1),
				(float)GetValueFromJsonData(matrixArray[5], -1),
				(float)GetValueFromJsonData(matrixArray[6], -1),
				(float)GetValueFromJsonData(matrixArray[7], -1)
			);
			Vec4 kBasis = Vec4
			(
				(float)GetValueFromJsonData(matrixArray[8], -1),
				(float)GetValueFromJsonData(matrixArray[9], -1),
				(float)GetValueFromJsonData(matrixArray[10], -1),
				(float)GetValueFromJsonData(matrixArray[11], -1)
			);
			Vec4 translation = Vec4
			(
				(float)GetValueFromJsonData(matrixArray[12], -1),
				(float)GetValueFromJsonData(matrixArray[13], -1),
				(float)GetValueFromJsonData(matrixArray[14], -1),
				(float)GetValueFromJsonData(matrixArray[15], -1)
			);
			newObject->m_localMatrix = Mat44(iBasis, jBasis, kBasis, translation);
			newObject->m_globalMatrix = Mat44(iBasis, jBasis, kBasis, translation);
		}
		if (DoesValueExist(currentData, "rotation"))
		{
			JsonArray rotationArray = GetArrayFromJsonData(GetValueFromJsonData(currentData, "rotation"));
			newObject->m_rotation = Vec4
			(
				(float)GetValueFromJsonData(rotationArray[0], -1),
				(float)GetValueFromJsonData(rotationArray[1], -1),
				(float)GetValueFromJsonData(rotationArray[2], -1),
				(float)GetValueFromJsonData(rotationArray[3], -1)
			);
		}
		if (DoesValueExist(currentData, "scale"))
		{
			JsonArray scaleArray = GetArrayFromJsonData(GetValueFromJsonData(currentData, "scale"));
			newObject->m_scale = Vec3
			(
				(float)GetValueFromJsonData(scaleArray[0], -1),
				(float)GetValueFromJsonData(scaleArray[1], -1),
				(float)GetValueFromJsonData(scaleArray[2], -1)
			);
		}
		if (DoesValueExist(currentData, "translation"))
		{
			JsonArray translationArray = GetArrayFromJsonData(GetValueFromJsonData(currentData, "translation"));
			newObject->m_translation = Vec3
			(
				(float)GetValueFromJsonData(translationArray[0], -1),
				(float)GetValueFromJsonData(translationArray[1], -1),
				(float)GetValueFromJsonData(translationArray[2], -1)
			);
		}

		m_nodes.push_back(newObject);
	}
}

void glTF_Asset::Initialize_LoadAllMeshes(JsonData  const& jsonData)
{
	JsonData meshes = GetValueFromJsonData(jsonData, "meshes");
	JsonArray meshesArray = GetArrayFromJsonData(meshes);
	for (int arrayIndex = 0; arrayIndex < meshesArray.size(); ++arrayIndex)
	{
		glTF_Mesh* newObject = new glTF_Mesh();
		JsonData currentData = meshesArray[arrayIndex];

		newObject->m_name = GetValueFromJsonData(currentData, "name", "");

		// For each primitive
		JsonArray primitiveIndexArray = GetArrayFromJsonData(GetValueFromJsonData(currentData, "primitives"));
		for (int primitiveIndex = 0; primitiveIndex < primitiveIndexArray.size(); ++primitiveIndex)
		{
			glTF_Primitive* newPrimitive = new glTF_Primitive();
			JsonData primitiveAttributes = GetValueFromJsonData(primitiveIndexArray[primitiveIndex], "attributes");
			JsonMap primitiveAttributesMap = GetMapFromJsonData(primitiveAttributes);
			JsonMap::iterator it;
			for (it = primitiveAttributesMap.begin(); it != primitiveAttributesMap.end(); ++it)
			{
				newPrimitive->m_attributes[it->first] = (int)it->second.get<double>();
			}

			newPrimitive->m_indicesIndex = GetValueFromJsonData(primitiveIndexArray[primitiveIndex], "indices", -1);
			newPrimitive->m_materialIndex = GetValueFromJsonData(primitiveIndexArray[primitiveIndex], "material", -1);
			newPrimitive->m_modeIndex = GetValueFromJsonData(primitiveIndexArray[primitiveIndex], "mode", -1);

			newObject->m_primitives.push_back(newPrimitive);
		}

		m_meshes.push_back(newObject);
	}
}

void glTF_Asset::Initialize_LoadAllMaterials(JsonData  const& jsonData)
{
	JsonData materials = GetValueFromJsonData(jsonData, "materials");
	JsonArray materialsArray = GetArrayFromJsonData(materials);
	for (int arrayIndex = 0; arrayIndex < materialsArray.size(); ++arrayIndex)
	{
		glTF_Material* newObject = new glTF_Material();
		JsonData currentData = materialsArray[arrayIndex];

		newObject->m_name = GetValueFromJsonData(currentData, "name", "");
		newObject->m_alphaMode = GetValueFromJsonData(currentData, "alphaMode", "");
		if (DoesValueExist(currentData, "pbrMetallicRoughness"))
		{
			glTF_pbrMetallicRoughness* pbrMR = new glTF_pbrMetallicRoughness();
			JsonData pbrMetallicRoughness = GetValueFromJsonData(currentData, "pbrMetallicRoughness");

			if (DoesValueExist(pbrMetallicRoughness, "baseColorFactor"))
			{
				JsonArray baseColorFactorArray = GetArrayFromJsonData(GetValueFromJsonData(pbrMetallicRoughness, "baseColorFactor"));
				pbrMR->m_baseColorFactor = Vec4
				(
					(float)GetValueFromJsonData(baseColorFactorArray[0], -1.0),
					(float)GetValueFromJsonData(baseColorFactorArray[1], -1.0),
					(float)GetValueFromJsonData(baseColorFactorArray[2], -1.0),
					(float)GetValueFromJsonData(baseColorFactorArray[3], -1.0)
				);
			}
			else
			{
				pbrMR->m_baseColorFactor = Vec4(1.f, 1.f, 1.f, 1.f);
			}

			if (DoesValueExist(pbrMetallicRoughness, "baseColorTexture"))
			{
				JsonData baseColorTexture = GetValueFromJsonData(pbrMetallicRoughness, "baseColorTexture");
				pbrMR->m_baseColorTextureIndex = GetValueFromJsonData(baseColorTexture, "index", -1);
			}

			if (DoesValueExist(pbrMetallicRoughness, "metallicRoughnessTexture"))
			{
				JsonData metallicroughnessTexture = GetValueFromJsonData(pbrMetallicRoughness, "metallicRoughnessTexture");
				pbrMR->m_metallicRougnessTextureIndex = GetValueFromJsonData(metallicroughnessTexture, "index", -1);
				pbrMR->m_metallicFactor = GetValueFromJsonData(pbrMetallicRoughness, "metallicFactor", -1.0);
				pbrMR->m_roughnessFactor = GetValueFromJsonData(pbrMetallicRoughness, "roughnessFactor", -1.0);
			}

			newObject->m_pbrMetallicRoughness = pbrMR;
		}
		if (DoesValueExist(currentData, "normalTexture"))
		{
			newObject->m_normalTextureIndex = GetValueFromJsonData(GetValueFromJsonData(currentData, "normalTexture"), "index", -1);
		}
		if (DoesValueExist(currentData, "occlusionTexture"))
		{
			newObject->m_occlusionTextureIndex = GetValueFromJsonData(GetValueFromJsonData(currentData, "occlusionTexture"), "index", -1);
		}
		if (DoesValueExist(currentData, "emissiveTexture"))
		{
			newObject->m_emissiveTextureIndex = GetValueFromJsonData(GetValueFromJsonData(currentData, "emissiveTexture"), "index", -1);
		}

		m_materials.push_back(newObject);
	}
}

void glTF_Asset::Initialize_LoadAllTextures(JsonData const& jsonData)
{
	JsonData textures = GetValueFromJsonData(jsonData, "textures");
	JsonArray texturesArray = GetArrayFromJsonData(textures);
	for (int arrayIndex = 0; arrayIndex < texturesArray.size(); ++arrayIndex)
	{
		glTF_Texture* newObject = new glTF_Texture();
		JsonData currentData = texturesArray[arrayIndex];

		newObject->m_imageIndex = GetValueFromJsonData(currentData, "source", -1);
		newObject->m_samplerIndex = GetValueFromJsonData(currentData, "sampler", -1);

		m_textures.push_back(newObject);
	}
}

void glTF_Asset::Initialize_LoadAllImages(JsonData const& jsonData)
{
	JsonData images = GetValueFromJsonData(jsonData, "images");
	JsonArray imagesArray = GetArrayFromJsonData(images);
	for (int arrayIndex = 0; arrayIndex < imagesArray.size(); ++arrayIndex)
	{
		glTF_Image* newObject = new glTF_Image();
		JsonData currentData = imagesArray[arrayIndex];

		newObject->m_uri = GetValueFromJsonData(currentData, "uri", "");

		Image* newImage = new Image((m_path + newObject->m_uri).data());

		Texture* newTexture = g_engine->m_render->CreateOrGetTextureFromFile((m_path + newObject->m_uri).data());

		newObject->m_texture = newTexture;
		newObject->m_image = newImage;

		m_images.push_back(newObject);
	}
}

void glTF_Asset::Initialize_LoadAllSamplers(JsonData const& jsonData)
{
	JsonData samplers = GetValueFromJsonData(jsonData, "samplers");
	JsonArray samplersArray = GetArrayFromJsonData(samplers);
	for (int arrayIndex = 0; arrayIndex < samplersArray.size(); ++arrayIndex)
	{
		glTF_Sampler* newObject = new glTF_Sampler();
		JsonData currentData = samplersArray[arrayIndex];

		// TODO: Implement a separate mode for U and V, right now it just defaults to the U wrapping mode.
		int samplerID = GetValueFromJsonData(currentData, "wrapS", -1);
		switch (samplerID) // TODO: Implement sampler modes
		{
			case 10497: newObject->m_samplerMode = SamplerMode::BILINEAR_WRAP; break;
			case 33071: newObject->m_samplerMode = SamplerMode::POINT_CLAMP; break;
			//case 33648: newObject->m_samplerMode = SamplerMode::BILINEAR_WRAP; break;
		}
		newObject->m_samplerMode;

		m_samplers.push_back(newObject);
	}
}

void glTF_Asset::Initialize_LoadBufferInformationToMeshes()
{
	for (int meshIndex = 0; meshIndex < m_meshes.size(); ++meshIndex)
	{
		glTF_Mesh* currentMesh = m_meshes[meshIndex];
		for (int primitiveIndex = 0; primitiveIndex < currentMesh->m_primitives.size(); ++primitiveIndex) // TODO: write helper function to take in accessor and return the bytes for it.
		{
			glTF_Primitive* currentPrimitive = currentMesh->m_primitives[primitiveIndex];

			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			// Indices
			// Not using the helper function so we can get indicesNumBytesPerElement
			glTF_Accessor* indicesAccessor = m_accessors[currentPrimitive->m_indicesIndex];
			glTF_BufferView* indicesBufferView =m_bufferViews[indicesAccessor->m_bufferViewIndex];
			glTF_Buffer* currentBuffer = m_buffers[indicesBufferView->m_bufferIndex];

			int indicesNumBytesPerElement = GLTF_GetNumBytesFromComponentType(indicesAccessor->m_componentType);
			int indicesNumComponentsPerElement = GLTF_GetNumComponentsFromType(indicesAccessor->m_type);
			int start = indicesAccessor->m_byteOffset + indicesBufferView->m_byteOffset;
			int end = start + (indicesAccessor->m_count * indicesNumBytesPerElement * indicesNumComponentsPerElement);

			currentPrimitive->m_indexes.assign(currentBuffer->m_data.begin() + start, currentBuffer->m_data.begin() + end);

			// Assign index type based on num of bytes per element
			if (indicesNumBytesPerElement == 1)
			{
				currentPrimitive->m_indexType = glTF_Primitive::IndexType::INDEXTYPE_8;
			}
			else if (indicesNumBytesPerElement == 2)
			{
				currentPrimitive->m_indexType = glTF_Primitive::IndexType::INDEXTYPE_16;
			}
			else if (indicesNumBytesPerElement == 4)
			{
				currentPrimitive->m_indexType = glTF_Primitive::IndexType::INDEXTYPE_32;
			}
			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			// Vertex data
			// Positions
			std::vector<uint8_t> positionsBuffer;
			glTF_Accessor* positionsAccessor = m_accessors[currentPrimitive->m_attributes["POSITION"]];
			positionsBuffer = ReadBytesFromAccessor(*positionsAccessor);

			// Parse the buffers to get the vertex positions
			// TODO: Write code to account for different data types other than floats? currently assumes the component type for positions and texcoords is 5126, or floats.
			float* positionsBufferAsFloat = reinterpret_cast<float*>(positionsBuffer.data());
			std::vector<Vec3> positionsAsVectors;
			for (int vectorIndex = 0; vectorIndex < positionsAccessor->m_count; ++vectorIndex)
			{
				Vec3 vector;
				vector.x = positionsBufferAsFloat[vectorIndex * 3 + 0];
				vector.y = positionsBufferAsFloat[vectorIndex * 3 + 1];
				vector.z = positionsBufferAsFloat[vectorIndex * 3 + 2];
				positionsAsVectors.push_back(vector);
			}

			// Normals
			std::vector<uint8_t> normalsBuffer;
			glTF_Accessor* normalsAccessor = m_accessors[currentPrimitive->m_attributes["NORMAL"]];
			normalsBuffer = ReadBytesFromAccessor(*normalsAccessor);

			// Parse the buffers to get the vertex positions
			// TODO: Write code to account for different data types other than floats? currently assumes the component type for positions and texcoords is 5126, or floats.
			float* normalsBufferAsFloat = reinterpret_cast<float*>(normalsBuffer.data());
			std::vector<Vec3> normalsAsVectors;
			for (int vectorIndex = 0; vectorIndex < normalsAccessor->m_count; ++vectorIndex)
			{
				Vec3 vector;
				vector.x = normalsBufferAsFloat[vectorIndex * 3 + 0];
				vector.y = normalsBufferAsFloat[vectorIndex * 3 + 1];
				vector.z = normalsBufferAsFloat[vectorIndex * 3 + 2];
				normalsAsVectors.push_back(vector);
			}

			// Texcoord
			// THERE ARE MULTIPLE TEXCOORDS IN A PRIMITIVE FOR THINGS LIKE NORMAL MAPS, WILL NEED TO WRITE EXTRA CODE TO ACCOUNT FOR IDENTIFYING WHICH TEXTURE GOES WHERE
			std::vector<uint8_t> texcoordBuffer;
			glTF_Accessor* texcoordAccessor = m_accessors[currentPrimitive->m_attributes["TEXCOORD_0"]];
			texcoordBuffer = ReadBytesFromAccessor(*texcoordAccessor);

			// Parse the buffers to get the vertex uvs
			float* texcoordBufferAsFloat = reinterpret_cast<float*>(texcoordBuffer.data());
			std::vector<Vec2> texcoordAsVectors;
			for (int vectorIndex = 0; vectorIndex < positionsAccessor->m_count; ++vectorIndex)
			{
				Vec2 vector;
				vector.x = texcoordBufferAsFloat[vectorIndex * 2 + 0];
				vector.y = 1.f - texcoordBufferAsFloat[vectorIndex * 2 + 1]; // TODO: Find out if the flipping is necessary for every gltf model, or if its specified somewhere in each file.
				texcoordAsVectors.push_back(vector);
			}
			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			// Populate vertexes.
			for (int vertexIndex = 0; vertexIndex < positionsAsVectors.size(); ++vertexIndex)
			{
				Vertex_PCUTBN newVertex = Vertex_PCUTBN(
					positionsAsVectors[vertexIndex],
					Rgba8::WHITE, // TODO: Get proper color from primitive data.
					texcoordAsVectors[vertexIndex],
					Vec3(),
					Vec3(),
					normalsAsVectors[vertexIndex]
				);
				currentPrimitive->m_verts.push_back(newVertex);
			}
			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		}
	}
}

void glTF_Asset::Initialize_LoadBufferInformationForAnimations()
{
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Test
	// This throws errors if theres no animations I believe.
	//int jointAccessorIndex = m_meshes[2]->m_primitives[0]->m_attributes["JOINTS_0"];
	//glTF_Accessor* jointAccessor = m_accessors[jointAccessorIndex];
	//std::vector<uint8_t> jointsBuffer = ReadBytesFromAccessor(*jointAccessor);
	//uint16_t* jointsBufferAs16bitArray = reinterpret_cast<uint16_t*>(jointsBuffer.data());

	//int sizeOfType = (int)jointsBuffer.size() / jointAccessor->m_count;
	//unsigned short firstJoint = jointsBufferAs16bitArray[0];
	//unsigned short lastJoint = jointsBufferAs16bitArray[(jointAccessor->m_count * 4) - 1];

	//int weightAccessorIndex = m_meshes[2]->m_primitives[0]->m_attributes["WEIGHTS_0"];
	//glTF_Accessor* weightAccessor = m_accessors[weightAccessorIndex];
	//std::vector<uint8_t> weightsBuffer = ReadBytesFromAccessor(*weightAccessor);
	//float* weightsBufferAsFloatsArray = reinterpret_cast<float*>(weightsBuffer.data());

	//int sizeOfTypeWeight = (int)weightsBuffer.size() / weightAccessor->m_count;
	//unsigned short firstWeight = (unsigned short)weightsBufferAsFloatsArray[0];
	//unsigned short lastWeight = (unsigned short)weightsBufferAsFloatsArray[(weightAccessor->m_count * 4) - 1];
	//int i = 0;
}

std::vector<uint8_t> glTF_Asset::ReadBytesFromAccessor(glTF_Accessor const& accessor)
{
	std::vector<uint8_t> returnBuffer;
	glTF_BufferView* bufferView = m_bufferViews[accessor.m_bufferViewIndex];
	glTF_Buffer* currentBuffer = m_buffers[bufferView->m_bufferIndex];

	int numBytesPerElement = GLTF_GetNumBytesFromComponentType(accessor.m_componentType);
	int numComponentsPerElement = GLTF_GetNumComponentsFromType(accessor.m_type);
	int start = accessor.m_byteOffset + bufferView->m_byteOffset;
	int end = start + (accessor.m_count * numBytesPerElement * numComponentsPerElement);

	returnBuffer.assign(currentBuffer->m_data.begin() + start, currentBuffer->m_data.begin() + end);

	return returnBuffer;
}

void glTF_Asset::Initialize_LoadAllAccessors(JsonData const& jsonData)
{
	JsonData accessors = GetValueFromJsonData(jsonData, "accessors");
	JsonArray accessorsArray = GetArrayFromJsonData(accessors);
	for (int arrayIndex = 0; arrayIndex < accessorsArray.size(); ++arrayIndex)
	{
		glTF_Accessor* newObject = new glTF_Accessor();
		JsonData currentData = accessorsArray[arrayIndex];

		newObject->m_bufferViewIndex = GetValueFromJsonData(currentData, "bufferView", -1);
		newObject->m_componentType = GetValueFromJsonData(currentData, "componentType", -1);
		newObject->m_count = GetValueFromJsonData(currentData, "count", 0);
		newObject->m_type = GetValueFromJsonData(currentData, "type", "");
		newObject->m_byteOffset = GetValueFromJsonData(currentData, "byteOffset", 0);
		if (DoesValueExist(currentData, "min"))
		{
			JsonArray jsonMins = GetArrayFromJsonData(GetValueFromJsonData(currentData, "min"));
			for (int minIndex = 0; minIndex < jsonMins.size(); ++minIndex)
			{
				newObject->m_mins.push_back(GetValueFromJsonData(jsonMins[minIndex], -1.0));
			}
		}
		if (DoesValueExist(currentData, "max"))
		{
			JsonArray jsonMaxs = GetArrayFromJsonData(GetValueFromJsonData(currentData, "max"));
			for (int maxIndex = 0; maxIndex < jsonMaxs.size(); ++maxIndex)
			{
				newObject->m_maxs.push_back(GetValueFromJsonData(jsonMaxs[maxIndex], -1.0));
			}
		}

		m_accessors.push_back(newObject);
	}
}

void glTF_Asset::Initialize_LoadAllBufferViews(JsonData const& jsonData)
{
	JsonData bufferViews = GetValueFromJsonData(jsonData, "bufferViews");
	JsonArray bufferViewsArray = GetArrayFromJsonData(bufferViews);
	for (int arrayIndex = 0; arrayIndex < bufferViewsArray.size(); ++arrayIndex)
	{
		glTF_BufferView* newObject = new glTF_BufferView();
		JsonData currentData = bufferViewsArray[arrayIndex];

		newObject->m_name = GetValueFromJsonData(currentData, "name", "");
		newObject->m_bufferIndex = GetValueFromJsonData(currentData, "buffer", -1);
		newObject->m_byteLength = GetValueFromJsonData(currentData, "byteLength", 0);
		newObject->m_byteStride = GetValueFromJsonData(currentData, "byteStride", 0);
		newObject->m_byteOffset = GetValueFromJsonData(currentData, "byteOffset", 0);
		newObject->m_target = GetValueFromJsonData(currentData, "target", -1);

		m_bufferViews.push_back(newObject);
	}
}

void glTF_Asset::Initialize_LoadAllBuffers(JsonData const& jsonData)
{
	JsonData buffers = GetValueFromJsonData(jsonData, "buffers");
	JsonArray buffersArray = GetArrayFromJsonData(buffers);
	for (int arrayIndex = 0; arrayIndex < buffersArray.size(); ++arrayIndex)
	{
		glTF_Buffer* newObject = new glTF_Buffer();
		JsonData currentData = buffersArray[arrayIndex];

		newObject->m_byteLength = GetValueFromJsonData(currentData, "byteLength", -1);
		newObject->m_uri = GetValueFromJsonData(currentData, "uri", "");

		m_buffers.push_back(newObject);
	}
}

void glTF_Asset::Test_RenderModel()
{
	//g_engine->m_render->BindShader(m_glTFAnimatedShader);
	for (int sceneIndex = 0; sceneIndex < m_scenes.size(); ++sceneIndex)
	{
		glTF_Scene* scene = m_scenes[sceneIndex];
		for (int sceneNodeIndex = 0; sceneNodeIndex < scene->m_nodeIndexes.size(); ++sceneNodeIndex)
		{
			int nodeIndex = scene->m_nodeIndexes[sceneNodeIndex];
			m_nodes[nodeIndex]->RenderNode();
		}
	}
	g_engine->m_render->BindShader(g_engine->m_render->m_defaultShader);
}

void glTF_Node::UpdateNode([[maybe_unused]] Mat44& parentMatrix)
{

}

void glTF_Node::RenderNode()
{
	// Render mesh if present.
	if (m_meshIndex != -1)
	{
		// Handle reading mesh and placing verts
		glTF_Mesh* mesh = m_owner->m_meshes[m_meshIndex];
		// Loop through all primitives in the mesh
		for (int primitiveIndex = 0; primitiveIndex < mesh->m_primitives.size(); ++primitiveIndex)
		{
			glTF_Primitive* currentPrimitive = mesh->m_primitives[primitiveIndex];

			// Bind correct texture
			if (currentPrimitive->m_materialIndex != -1 && m_owner->m_materials[currentPrimitive->m_materialIndex]->m_pbrMetallicRoughness != nullptr)
			{
				int textureIndex = m_owner->m_materials[currentPrimitive->m_materialIndex]->m_pbrMetallicRoughness->m_baseColorTextureIndex;
				int imageIndex = m_owner->m_textures[textureIndex]->m_imageIndex;
				Texture* texture = m_owner->m_images[imageIndex]->m_texture;
				g_engine->m_render->BindTexture(texture);

				// Swap sampler mode
				int samplerIndex = m_owner->m_textures[textureIndex]->m_samplerIndex;
				glTF_Sampler* sampler = m_owner->m_samplers[samplerIndex];
				g_engine->m_render->SetSamplerMode(sampler->m_samplerMode);
			}
			else
			{
				g_engine->m_render->BindTexture(nullptr);
			}

			switch (currentPrimitive->m_indexType)
			{
				case glTF_Primitive::IndexType::INDEXTYPE_8: // TODO: Find a way to convert uint8_t and uint16_t into unsigned int stored with 32 bits. current implementation of dividing may be flawed.
				{
					// TODO: Have this conversion take place when the model is loaded, so that we dont have to handle the logic during render
					std::vector<uint32_t> indexesAs32Bit;
					uint8_t* src = reinterpret_cast<uint8_t*>(currentPrimitive->m_indexes.data());
					size_t indexCount = currentPrimitive->m_indexes.size() / sizeof(uint8_t);
					indexesAs32Bit.resize(indexCount);
					for (size_t indexIndex = 0; indexIndex < indexCount; indexIndex++)
					{
						indexesAs32Bit[indexIndex] = src[indexIndex];
					}

					g_engine->m_render->DrawIndexedVertexArray(
						(int)currentPrimitive->m_verts.size(),
						currentPrimitive->m_verts.data(),
						(int)indexesAs32Bit.size(),
						indexesAs32Bit.data()
					);
					break;
				}
				case glTF_Primitive::IndexType::INDEXTYPE_16:
				{
					// TODO: Have this conversion take place when the model is loaded, so that we dont have to handle the logic during render
					std::vector<uint32_t> indexesAs32Bit;
					uint16_t* src = reinterpret_cast<uint16_t*>(currentPrimitive->m_indexes.data());
					size_t indexCount = currentPrimitive->m_indexes.size() / sizeof(uint16_t);
					indexesAs32Bit.resize(indexCount);
					for (size_t indexIndex = 0; indexIndex < indexCount; indexIndex++)
					{
						indexesAs32Bit[indexIndex] = src[indexIndex];
					}

					g_engine->m_render->DrawIndexedVertexArray(
						(int)currentPrimitive->m_verts.size(),
						currentPrimitive->m_verts.data(),
						(int)indexesAs32Bit.size(),
						indexesAs32Bit.data()
					);
					break;
				}
				case glTF_Primitive::IndexType::INDEXTYPE_32:
				{
					g_engine->m_render->DrawIndexedVertexArray(
						(int)currentPrimitive->m_verts.size(),
						currentPrimitive->m_verts.data(),
						(int)currentPrimitive->m_indexes.size() / sizeof(unsigned int),
						reinterpret_cast<unsigned int*>(currentPrimitive->m_indexes.data())
					);
					break;
				}
			}
		}
	}

	// Loop through children to render.
	for (int childIndexIndex = 0; childIndexIndex < m_childIndexes.size(); ++childIndexIndex)
	{
		int currentChildIndex = m_childIndexes[childIndexIndex];
		m_owner->m_nodes[currentChildIndex]->RenderNode();
	}
}

int GLTF_GetNumBytesFromComponentType(int componentType)
{
	switch (componentType)
	{
		case 5120: return 1;
		case 5121: return 1;
		case 5122: return 2;
		case 5123: return 2;
		case 5125: return 4;
		case 5126: return 4;
	}
	return -1;
}

int GLTF_GetNumComponentsFromType(std::string type)
{
	if (type == std::string("SCALAR"))
	{
		return 1;
	}
	if (type == std::string("VEC2"))
	{
		return 2;
	}
	if (type == std::string("VEC3"))
	{
		return 3;
	}
	if (type == std::string("VEC4"))
	{
		return 4;
	}
	if (type == std::string("MAT2"))
	{
		return 4;
	}
	if (type == std::string("MAT3"))
	{
		return 9;
	}
	if (type == std::string("MAT4"))
	{
		return 16;
	}
	return -1;
}
