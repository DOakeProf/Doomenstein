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
cbuffer LightConstants : register(b1)
{
    float3 SunDirection;
    float SunIntensity;
    float AmbientIntensity;
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform; // View transform
    float4x4 CameraToRenderTransform; // Non-standard transform from game to DirectX conventions
    float4x4 RenderToClipTransform; // Projection transform
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform; // Model transform
    float4 ModelColor;
};

cbuffer ClipPlaneConstants : register(b5)
{
    float4 gClipPlane;
    int isEnabled;
    float3 padding;
};

cbuffer PortalAABB3Constants : register(b6)
{
    float3 aabb3Mins;
    int isEnabledAABB3;
    float3 aabb3Maxs;
    int paddingAABB3;
}

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);

//------------------------------------------------------------------------------------------------
SamplerState samplerState : register(s0);

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
    float4 modelPosition = float4(input.modelPosition, 1);
    float4 worldPosition = mul(ModelToWorldTransform, modelPosition);
    float4 cameraPosition = mul(WorldToCameraTransform, worldPosition);
    float4 renderPosition = mul(CameraToRenderTransform, cameraPosition);
    float4 clipPosition = mul(RenderToClipTransform, renderPosition);

    float4 worldTangent = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));
    float4 worldBitangent = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));
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
    float directional = SunIntensity * saturate(dot(normalize(input.worldNormal.xyz), -SunDirection));
    float4 lightColor = float4((ambient + directional).xxx, 1);
    float4 textureColor = diffuseTexture.Sample(samplerState, input.uv);
    float4 vertexColor = input.color;
    float4 modelColor = ModelColor;
    float4 color = lightColor * textureColor * vertexColor * modelColor;
    clip(color.a - 0.01f);
    
    float distance = dot(input.worldPosition.xyz, gClipPlane.xyz) + gClipPlane.w;
    if (isEnabled)
    {
        clip(distance);
    }
    
    float isAboveXMin = input.worldPosition.x - aabb3Mins.x;
    float isBelowXMax = aabb3Maxs.x - input.worldPosition.x;
    float isAboveYMin = input.worldPosition.y - aabb3Mins.y;
    float isBelowYMax = aabb3Maxs.y - input.worldPosition.y;
    float isAboveZMin = input.worldPosition.z - aabb3Mins.z;
    float isAboveZMax = aabb3Maxs.z - input.worldPosition.z;
    if (isEnabledAABB3)
    {
        clip(isAboveXMin * isBelowXMax * isAboveYMin * isBelowYMax * isAboveZMin * isAboveZMax);
    }
    
    // Try passing in an AABB3 constant buffer and clip anything inside the portals AABB3
    
    return color;
}
