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
#define mod(x, y) (x - y * floor(x / y))

float3x3 SetCamera(in float3 ro, in float3 rt, in float cr)
{
	float3 cw = normalize(rt - ro);
	float3 cp = float3(sin(cr), cos(cr), 0.0);
	float3 cu = normalize(cross(cw, cp));
	float3 cv = normalize(cross(cu, cw));
	return float3x3(cu, cv, -cw);
}

float Fractal(float3 p)
{
	float3 w = p;
	float3 q = p;

	q.xz = mod(q.xz + 5.0, 2.0) - 1.0;

	float d = Box(q, 10.0);
	float s = 1.0;
	[unroll]
	for (int m = 0; m<6; m++)
	{
		float h = float(m) / 6.0;

		p = q - 0.5*sin(abs(p.y) + float(m)*3.0 + float3(0.0, 3.0, 1.0));

		float3 a = mod(p*s, 2.0) - 1.0;
		s *= 3.0;
		float3 r = abs(1.0 - 3.0*abs(a));

		float da = max(r.x, r.y);
		float db = max(r.y, r.z);
		float dc = max(r.z, r.x);
		float c = (min(da, min(db, dc)) - 1.0) / s;

		d = max(c, d);
	}


	float d1 = length(w - float3(0.22, 0.35, 0.4)) - 0.09;
	d = min(d, d1);

	//float d2 = Plane(p - float3(0, 1, 0), float3(0, -1, 0), 1);
	//d = min(d, d2);


	return d;
}
SSceneHitInfo scene(float3 pos)
{
	SSceneHitInfo info;
	info.distance = Fractal(pos);
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


float4 main(float4 position : SV_POSITION, float2 fragmentCoordinate : TEXCOORD) : SV_TARGET
{
	float2 resolution = float2(1280.0, 720.0);
	float2 uv = (position.xy - resolution.xy * 0.5) / resolution.y;

	float3 raypos = float3(0.0, 0.0, -sin(time * 0.1) * 2);
	//float3 raydir = float3(uv.xy, 1.0);

	float3 lookat = float3(0.0,0,0);

	float3x3 camera = SetCamera(raypos, lookat, 0.0);
	float3 raydir = normalize(mul(camera, float3(uv, -1.3)));

	SSceneHitInfo result;
	int iresult = 0.0;

	for (int i = 0; i <= 100; ++i)
	{
		iresult = i;
		result = scene(raypos);
		raypos += raydir * result.distance * 0.99;
		if (result.distance < 0.01)
		{
			break;
		}
	}
	if (iresult == 100)
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