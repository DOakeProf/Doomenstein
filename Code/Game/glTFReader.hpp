#pragma once
#include <string>
#include <vector>

#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/FileParsers/JsonUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Renderer/Renderer.hpp"

struct	Vertex;
class	glTF_Asset;

struct glTF_Scene
{
	std::string m_name;
	std::vector<int> m_nodeIndexes;
};

struct glTF_Node
{
	glTF_Asset* m_owner = nullptr;

	std::string m_name;
	std::vector<int> m_childIndexes;
	int m_meshIndex;
	Vec3 m_translation;
	Vec3 m_scale;
	Vec4 m_rotation;
	float m_randomNum = 0.f;

	Mat44 m_localMatrix;
	Mat44 m_globalMatrix;

	void UpdateNode(Mat44& parentMatrix); // Every frame updates global and local transform matrixes according to animation changes?

	void AddVerts(std::vector<Vertex>& verts, std::vector<unsigned int>& indexes, Mat44& matrix);
	void RenderNode();
};

struct glTF_Primitive
{
	enum IndexType
	{
		INDEXTYPE_NONE = -1,
		INDEXTYPE_8,
		INDEXTYPE_16,
		INDEXTYPE_32,
		INDEXTYPE_COUNT
	};

	std::map<std::string, int> m_attributes; // POSITION stores an accessor index to the vertex position data
	int m_indicesIndex = 0; // Stores an accessor index to the triangles which all point to position index data
	int m_materialIndex = 0;
	int m_modeIndex = 0;

	IndexType m_indexType;

	std::vector<Vertex> m_verts;
	std::vector<uint8_t> m_indexes;
};

struct glTF_Mesh
{
	std::string m_name;
	std::vector<glTF_Primitive*> m_primitives;
};

struct glTF_Accessor
{
	int m_bufferViewIndex = 0;
	int m_componentType = 0;
	int m_count = 0;
	int m_byteOffset = 0;
	std::string m_type;
	std::vector<double> m_mins;
	std::vector<double> m_maxs;
};

struct glTF_BufferView
{
	std::string m_name;
	int m_bufferIndex = 0;
	int m_byteLength = 0;
	int m_byteStride = 0;
	int m_byteOffset = 0;
	int m_target = 0;
};

struct glTF_Buffer
{
	int m_byteLength;
	std::string m_uri;
	std::vector<uint8_t> m_data;
};

struct glTF_pbrMetallicRoughness
{
	Vec4 m_baseColorFactor;
	double m_metallicFactor;
	double m_roughnessFactor;
	int m_baseColorTextureIndex;
	int m_metallicRougnessTextureIndex;
};

struct glTF_Material
{
	std::string m_name;
	std::string m_alphaMode;
	glTF_pbrMetallicRoughness* m_pbrMetallicRoughness;
	int m_normalTextureIndex;
	int m_occlusionTextureIndex;
	int m_emissiveTextureIndex;
};

struct glTF_Texture
{
	int m_samplerIndex;
	int m_imageIndex;
};

struct glTF_Image
{
	std::string m_uri;
	Image* m_image;
	Texture* m_texture;
};

struct glTF_Sampler
{
	SamplerMode m_samplerMode;
};

class glTF_Asset
{
public:
	glTF_Asset(const char* gltfFilePath, const char* gltfBinPath);
	~glTF_Asset();

	void Initialize_LoadAllScenes(JsonData const& jsonData);
	void Initialize_LoadAllNodes(JsonData const& jsonData);
	void Initialize_LoadAllMeshes(JsonData const& jsonData);
	void Initialize_LoadAllAccessors(JsonData const& jsonData);
	void Initialize_LoadAllBufferViews(JsonData const& jsonData);
	void Initialize_LoadAllBuffers(JsonData const& jsonData);

	void Initialize_LoadAllMaterials(JsonData const& jsonData);
	void Initialize_LoadAllTextures(JsonData const& jsonData);
	void Initialize_LoadAllImages(JsonData const& jsonData);
	void Initialize_LoadAllSamplers(JsonData const& jsonData);

	void Initialize_LoadBufferInformationToMeshes();
	void Initialize_LoadBufferInformationForAnimations();

	std::vector<uint8_t> ReadBytesFromAccessor(glTF_Accessor const& accessor);

	void Test_AddVertsForModel(std::vector<Vertex>& verts, std::vector<unsigned int>& indexes);

	void Test_RenderModel();

public:
	std::vector<glTF_Scene*> m_scenes;
	std::vector<glTF_Node*> m_nodes;
	std::vector<glTF_Mesh*> m_meshes;
	std::vector<glTF_Accessor*> m_accessors;
	std::vector<glTF_BufferView*> m_bufferViews;
	std::vector<glTF_Buffer*> m_buffers;
	std::vector<glTF_Material*> m_materials;
	std::vector<glTF_Texture*> m_textures;
	std::vector<glTF_Image*> m_images;
	std::vector<glTF_Sampler*> m_samplers;

	std::string m_path;

	Shader* m_glTFAnimatedShader = nullptr;
};

int GLTF_GetNumBytesFromComponentType(int componentType);
int GLTF_GetNumComponentsFromType(std::string type);