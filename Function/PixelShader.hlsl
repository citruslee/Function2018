#include "hg_sdf.hlsli"

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
		Box(pos, float3(10, 1, 10)), 
		Box(pos, float3(7, 0.9, 100))
	);
}

float2 scene(float3 pos)
{
	float2 box = float2(Hangar(pos), 1);
	return box;
}

float3 sceneNormal(float3 pos)
{
	float distancePoint = scene(pos).x;
	float aepsilon = 0.01;
	float x = scene(pos + float3(AEPSILON, 0.0, 0.0)).x;
	float y = scene(pos + float3(0.0, AEPSILON, 0.0)).x;
	float z = scene(pos + float3(0.0, 0.0, AEPSILON)).x;
	return normalize(float3(x - distancePoint, y - distancePoint, z - distancePoint));
}

float4 main(float4 position : SV_POSITION, float2 fragmentCoordinate : TEXCOORD) : SV_TARGET
{
	float2 resolution = float2(1280.0, 720.0);
	float2 uv = (position.xy - resolution.xy * 0.5) / resolution.y;

	float3 raypos = float3(0.0, 0.0, -5.0);
	float3 raydir = float3(uv.xy, 1.0);

	float2 result = 0.0;
	float iresult = 0.0;

	for (float i = 0.0; i < 50.0; i++)
	{
		iresult = i;
		result = scene(raypos);
		raypos += raydir * result.x * 0.99;
		if (result.x < 0.001)
		{
			return float4(sceneNormal(raypos), 1);
		}
	}
	return float4(1.0, 0.1, 0, 1);
}