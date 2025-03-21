#include "common_fxc.inc"

#define MATH_PI 3.14159265359

struct PushConstants
{
	float time;
//	int cbuffer_RID;
//  int texture2D_RID;
//  int cubeMap_RID;
};

[[vk::push_constant]]
PushConstants pushConstants;

#define MAX_N_LIGHTS 16

struct Light
{
    float3 position;
    float radius;
    float3 colour;
    float attenuation;
    float3 direction;
    float type; // 0 - sun, 1 - spotlight, 2 - point
};

struct FrameUBO
{
    float4x4 projMatrix;
    float4x4 viewMatrix;
    float4 viewPos;
    Light lights[MAX_N_LIGHTS];
};

ConstantBuffer<FrameUBO> frameData : register(b0);

struct InstanceUBO
{
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

ConstantBuffer<InstanceUBO> instanceData : register(b1);

struct MaterialUBO
{
    float temp;
};

ConstantBuffer<MaterialUBO> materialData : register(b2); 
