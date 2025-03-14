#include "common_ps.hlsl"

struct PSInput
{
    [[vk::location(VS_OUT_SLOT_POSITION)]]
	float3 position : TEXCOORD0;
	
    [[vk::location(VS_OUT_SLOT_UV)]]
	float2 texCoord : TEXCOORD1;
	
    [[vk::location(VS_OUT_SLOT_COLOUR)]]
	float3 colour : COLOR;
	
    [[vk::location(VS_OUT_SLOT_TANGENT_FRAG_POS)]]
	float3 fragPos : TEXCOORD2;
	
    [[vk::location(VS_OUT_SLOT_TBN_MATRIX)]]
	float3x3 tbn : TEXCOORD4;
};

#define MAX_REFLECTION_LOD 4.0

TextureCube localIrradianceMap : register(t3);
SamplerState localIrradianceMapSampler : register(s3);

TextureCube localPrefilterMap : register(t4);
SamplerState localPrefilterMapSampler : register(s4);

Texture2D brdfLUT : register(t5);
SamplerState brdfLUTSampler : register(s5);

Texture2D diffuseTexture : register(t6);
Texture2D aoTexture : register(t7);
Texture2D mrTexture : register(t8);
Texture2D normalTexture : register(t9);
Texture2D emissiveTexture : register(t10);

SamplerState diffuseSampler : register(s6);
SamplerState aoSampler : register(s7);
SamplerState mrSampler : register(s8);
SamplerState normalSampler : register(s9);
SamplerState emissiveSampler : register(s10);

float3 fresnelSchlick(float cosT, float3 F0, float roughness)
{
	float t = pow(1.0 - max(cosT, 0.0), 5.0);
	float3 F90 = max(1.0 - roughness, F0);
	return lerp(F0, F90, t);
}

float distributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	
	denom = max(denom, 0.00001);

	return a2 / (denom * denom * MATH_PI);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;
	
	return NdotV / lerp(k, 1.0, NdotV);
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
	float ggx2 = geometrySchlickGGX(NdotV, roughness);
	float ggx1 = geometrySchlickGGX(NdotL, roughness);
	
	return ggx1 * ggx2;
}

float4 main(PSInput input) : SV_Target
{
	float2 uv = frac(input.texCoord);
	
	float3 albedo = diffuseTexture.Sample(diffuseSampler, uv).rgb;
	float ambientOcclusion = aoTexture.Sample(aoSampler, uv).r;
	float3 metallicRoughness = mrTexture.Sample(mrSampler, uv).rgb;
	float3 normal = normalTexture.Sample(normalSampler, uv).rgb;
	float3 emissive = emissiveTexture.Sample(emissiveSampler, uv).rgb;
	
	ambientOcclusion += metallicRoughness.r;
	float roughnessValue = metallicRoughness.g;
	float metallicValue = metallicRoughness.b;
	
	float3 F0 = lerp(0.04, albedo, metallicValue);
	
	normal = normalize(mul(normalize(2.0*normal - 1.0), input.tbn));
	float3 viewDir = normalize(frameData.viewPos.xyz - input.fragPos);
	
	float NdotV = max(0.0, dot(normal, viewDir));
	
	float3 Lo = 0.0;
	
	for (int i = 0; i < MAX_N_LIGHTS; i++)
	{
		if (dot(frameData.lights[i].colour, frameData.lights[i].colour) <= 0.01)
			continue;
		
		float3 deltaX = frameData.lights[i].position - input.fragPos;
		float distanceSquared = dot(deltaX, deltaX);
		float attenuation = 1.0 / (0.125 * distanceSquared);
		float3 radiance = frameData.lights[i].colour * attenuation;
		
		float3 lightDir = normalize(deltaX);
		float3 halfwayDir = normalize(lightDir + viewDir);
	
		float3 reflected = reflect(-viewDir, normal);
		reflected = normalize(reflected);
	
		float NdotL = max(0.0, dot(normal, lightDir));
		float NdotH = max(0.0, dot(normal, halfwayDir));
		
		float cosT = max(0.0, dot(halfwayDir, viewDir));
		
		float3 F = fresnelSchlick(cosT, F0, 0.0);
		
		float NDF = distributionGGX(NdotH, roughnessValue);
		float G = geometrySmith(NdotV, NdotL, roughnessValue);
		
		float3 kD = (1.0 - F) * (1.0 - metallicValue);
		
		float3 diffuse = kD * albedo / MATH_PI;
		float3 specular = (F * G * NDF) / (4.0 * NdotL * NdotV + 0.0001);
		
		Lo += (diffuse + specular) * radiance * NdotL;
	}
	
	float3 F = fresnelSchlick(NdotV, F0, roughnessValue);
	float3 kD = (1.0 - F) * (1.0 - metallicValue);
	
	float3 reflected = reflect(-viewDir, normal);
	float3 prefilteredColour = localPrefilterMap.SampleLevel(localPrefilterMapSampler, reflected, roughnessValue * MAX_REFLECTION_LOD).rgb;
	float2 environmentBRDF = brdfLUT.Sample(brdfLUTSampler, float2(NdotV, roughnessValue)).xy;
	float3 specular = prefilteredColour * (F * environmentBRDF.x + environmentBRDF.y);
	
	float3 irradiance = localIrradianceMap.Sample(localIrradianceMapSampler, normal).rgb;
	float3 diffuse = irradiance * albedo;
	float3 ambient = (kD * diffuse + specular) * ambientOcclusion;
	
	float3 finalColour = ambient + Lo + emissive;
	
	finalColour *= input.colour;
	
	return float4(finalColour, 1.0);

	/*
	float3 camPos = frameData.viewPos.xyz;
	float3 worldPos = input.fragPos;
	
	float3 viewDir = normalize(camPos - worldPos);
	
	// we don't use the normal texture as we don't want that much detail - just geometry
	float3 normal = input.tbn[2];
	
	float NdotV = dot(normal, viewDir);
	float theta = acos(NdotV);
	
	float curvature = length(fwidth(normal)) / length(fwidth(worldPos));
	
	float radius = 1.0 / (curvature + 0.001);
	
	float diffusionCoeff = 1.0;
	float extinctionCoeff = 0.1;
	
	float alpha = 1.0;
	
	float3 A = 0.0;
	float3 B = 0.0;
	
	float dx = 0.01;
	
	for (float x = -MATH_PI; x <= MATH_PI; x += dx)
	{
		float3 irradianceSample = alpha / (4.0 * MATH_PI * diffusionCoeff) * exp(-abs(2.0 * sin(x * 0.5)) / diffusionCoeff);
		
		A += irradianceSample * dx * cos(theta + x);
		B += irradianceSample * dx;
	}
	
	float3 transmittance = exp(-extinctionCoeff * (A / B));
	
	return float4(transmittance, 1.0);
	*/
}
