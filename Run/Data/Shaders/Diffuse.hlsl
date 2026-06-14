//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 modelTangent : TANGENT;
	float3 modelBitangent : BITANGENT;
	float3 modelNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 clipPosition : SV_Position;
    float4 worldPosition : TEXCOORD1;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 worldTangent : TANGENT;
	float4 worldBitangent : BITANGENT;
	float4 worldNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 WorldToCameraTransform;	// View transform
	float4x4 CameraToRenderTransform;	// Non-standard transform from game to DirectX conventions
	float4x4 RenderToClipTransform;		// Projection transform
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelToWorldTransform;		// Model transform
	float4 ModelColor;
};

//------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b8)
{
	float3 SunDirection;
	float SunIntensity;
	float AmbientIntensity;
};

cbuffer ClipPlaneConstants : register(b9)
{
    float4 gClipPlane[5];
    int isEnabledClipPlane;
    int amountOfClipPlanes;
    float2 paddingClipPlane;
};

cbuffer PortalAABB3Constants : register(b10)
{
    float4 aabb3Mins[4];
    float4 aabb3Maxs[4];
    int isEnabledAABB3;
    int amountOfPortals;
};

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);

//------------------------------------------------------------------------------------------------
SamplerState samplerState : register(s0);

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float isPointInAABB(float3 pointToCheck, float3 mins, float3 maxs)
{
    float3 minCheck = step(mins, pointToCheck);
    float3 maxCheck = step(pointToCheck, maxs);
    float3 combined = minCheck * maxCheck;
	
    return combined.x * combined.y * combined.z;
}

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 modelPosition = float4(input.modelPosition, 1);
	float4 worldPosition = mul(ModelToWorldTransform, modelPosition);
	float4 cameraPosition = mul(WorldToCameraTransform, worldPosition);
	float4 renderPosition = mul(CameraToRenderTransform, cameraPosition);
	float4 clipPosition = mul(RenderToClipTransform, renderPosition);

	float4 worldTangent = mul(ModelToWorldTransform, float4(input.modelTangent, 0.0f));
	float4 worldBitangent = mul(ModelToWorldTransform, float4(input.modelBitangent, 0.0f));
	float4 worldNormal = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));

	v2p_t v2p;
	v2p.clipPosition = clipPosition;
    v2p.worldPosition = worldPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.worldTangent = worldTangent;
	v2p.worldBitangent = worldBitangent;
	v2p.worldNormal = worldNormal;
	return v2p;
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
	float ambient = AmbientIntensity;
    float directional = SunIntensity * saturate(dot(normalize(input.worldNormal.xyz), -normalize(SunDirection)));
	float4 lightColor = float4((ambient + directional).xxx, 1);
	float4 textureColor = diffuseTexture.Sample(samplerState, input.uv);
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;
	float4 color = lightColor * textureColor * vertexColor * modelColor;
	clip(color.a - 0.01f);
	
    for (int clipPlaneIndex = 0; clipPlaneIndex < amountOfClipPlanes; ++clipPlaneIndex)
    {
        float distance = dot(input.worldPosition.xyz, gClipPlane[clipPlaneIndex].xyz) + gClipPlane[clipPlaneIndex].w;
        if (isEnabledClipPlane)
        {
            clip(distance);
        }
    }
	
    for (int portalAABB3Index = 0; portalAABB3Index < amountOfPortals; ++portalAABB3Index)
    {
        float isPointInside = isPointInAABB(input.worldPosition.xyz, aabb3Mins[portalAABB3Index].xyz, aabb3Maxs[portalAABB3Index].xyz);
        if (isEnabledAABB3)
        {
            clip(0.5 - isPointInside);
        }
    }

    return color;
}

// return float4(aabb3Mins.x, aabb3Mins.y, aabb3Mins.z, 1.f);