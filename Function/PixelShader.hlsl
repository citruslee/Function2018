#include "hg_sdf.hlsli"

struct SSceneHitInfo
{
	float3 albedo;
	uint materialidx;
	float distance;
};

cbuffer ConstantData
{
	float time;
}

float2 Union(float2 Distance0, float2 Distance1)
{
	return (Distance0.x < Distance1.x) ? Distance0 : Distance1;
}

float Intersect(float a, float b)
{
	return max(a, -b);
}

float Hangar(float3 pos)
{
	return Intersect(
		Box(pos, float3(1, 1, 1)), 
		Box(pos, float3(0.5, 0.5, 0.5))
	);
}

SSceneHitInfo scene(float3 pos)
{
	SSceneHitInfo info;
	info.distance = length(pos + sin(time)) - 1;
	info.materialidx = 1;
	info.albedo = float3(1, 0.8, 1);
	return info;
}

float3 sceneNormal(float3 pos)
{
	float distancePoint = scene(pos).distance;
	float x = scene(pos + float3(AEPSILON, 0.0, 0.0)).distance;
	float y = scene(pos + float3(0.0, AEPSILON, 0.0)).distance;
	float z = scene(pos + float3(0.0, 0.0, AEPSILON)).distance;
	return normalize(float3(x - distancePoint, y - distancePoint, z - distancePoint));
}

float f(float3 pos)
{
	return length(pos + float3(0, 10, 0)) - 1;
}

float4 main(float4 position : SV_POSITION, float2 fragmentCoordinate : TEXCOORD) : SV_TARGET
{
	float2 resolution = float2(1280.0, 720.0);
	float2 uv = (position.xy - resolution.xy * 0.5) / resolution.y;

	float3 raypos = float3(0.0, 0.0, -5.0);
	float3 raydir = float3(uv.xy, 1.0);

	SSceneHitInfo result;
	int iresult = 0.0;

	for (int i = 0; i <= 50; ++i)
	{
		iresult = i;
		result = scene(raypos);
		raypos += raydir * result.distance * 0.99;
		if (result.distance < 0.01)
		{
			break;
		}
	}
	if (iresult == 50)
	{
		result.albedo = 0;
	}

	float3 surface = raypos + raydir*result.distance;
	float3 surfNormal = sceneNormal(surface);

	float3 lightpos = float3(1.0, -2.0, 0.0);
	float3 lightdir = lightpos - surface;
	float3 lcolor = float3(1.0, 1.0, 1.0);

	float len = length(lightdir);
	lightdir /= len;
	float3 reflected = reflect(-lightdir, surfNormal);

	float3 sceneColor = float3(0.0, 0.0, 0.0);
	float3 objColor = result.albedo;
	float ambient = 0.2;

	//diffuse
	float diffuse = max(0.0, dot(surfNormal, lightdir));

	// specular
	float specularPower = 5.0;
	float specular = max(0.0, dot(reflected, normalize(raypos - surface)));
	specular = pow(specular, specularPower);

	sceneColor += lcolor*1.0*objColor*(diffuse + specular + ambient);
	return float4(sceneColor.xyz, 1);
}