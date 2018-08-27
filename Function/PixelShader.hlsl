float scene(float3 pos)
{
	return length(pos) - 1;
}

float4 main(float4 position : SV_POSITION, float2 fragmentCoordinate : TEXCOORD) : SV_TARGET
{
	float2 resolution = float2(1280.0, 720.0);
	float2 uv = (position.xy - resolution.xy * 0.5) / resolution.y;

	float3 raypos = float3(0.0, 0.0, -5.0);
	float3 raydir = float3(uv.xy, 1.0);

	float result = 0.0;
	float iresult = 0.0;
	for (float i = 0.0; i < 100.0; i++)
	{
		iresult = i;
		result = scene(raypos);
		raypos += raydir * result * 0.7;
		if (result < 0.001)
		{
			return 1;
		}
	}
	return float4(uv, 0, 1);
}