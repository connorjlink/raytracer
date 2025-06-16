struct VS_IN
{
	float2 pos : POSITION;
};

struct VS_OUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

VS_OUT main(VS_IN input)
{
	VS_OUT result;
	result.pos = float4(input.pos, 0, 1);
	result.uv = input.pos.xy;
	return result;
}
