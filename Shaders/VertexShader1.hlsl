

struct PerCameraConstants
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 vpMatrix;
};
ConstantBuffer<PerCameraConstants> perCameraConstants : register(b0);




struct PerLightConstant
{
    float3 stength;
    float fallOffStart;
    float3 position;
    float fallOffEnd;
    float3 direction;
    float spotPower;
};
ConstantBuffer<PerLightConstant> perLightConstants: register(b1);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

PSInput main(float4 position : POSITION, float3 normal : NORMAL, float4 color: COLOR)
{
    PSInput result;
    result.position = mul(perCameraConstants.vpMatrix, position);
    result.color = color;
    result.normal = normal;

    return result;
}

