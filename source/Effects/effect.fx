//--------------------------------------
// Input/Output Structs
//--------------------------------------
struct VS_INPUT
{
    float3 Position         : POSITION;
    float3 WorldPosition    : COLOR;
    float3 Normal           : NORMAL;
    float3 Tangent          : TANGENT;
    float2 UV               : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Position         : SV_POSITION;
    float4 WorldPosition    : COLOR;
    float3 Normal           : NORMAL;
    float3 Tangent          : TANGENT;
    float2 UV               : TEXCOORD;
};

//--------------------------------------
// Global Variables
//--------------------------------------
float kd = 1.f; //Diffuse Reflection Coefficient
float lightIntensity = 7.f;
float shininess = 25.f;
float PI = 3.1415f;
float3 lightDirection = { .577f, -.577f, .577f };

float4x4 gWorldViewProj : WorldViewProjection;
float4x4 gWorldMatrix : World;
float4x4 gViewInverseMatrix : ViewInverse;

Texture2D gDiffuseMap : DiffuseMap;
Texture2D gNormalMap : NormalMap;
Texture2D gGlossinessMap : GlossinessMap;
Texture2D gSpecularMap : SpecularMap;

SamplerState samPoint
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap; // Or Mirror, Clamp, Border
    AddressV = Wrap; // Or Mirror, Clamp, Border
};

RasterizerState gRasterizerState
{
	CullMode = back;
	FrontCounterClockwise = false; // default
};

BlendState gBlendState
{
	BlendEnable[0] = false;
	RenderTargetWriteMask[0] = 0x0F;
};

DepthStencilState gDepthStencilState
{
	DepthEnable = true;
	DepthWriteMask = 1;
	DepthFunc = less;
	StencilEnable = false;
};

// //--------------------------------------
// // Vertex Shader
// //--------------------------------------

VS_OUTPUT VS(VS_INPUT input)
{
VS_OUTPUT output = (VS_OUTPUT)0;
output.Position = mul(float4(input.Position, 1.f), gWorldViewProj);
output.UV = input.UV;
output.WorldPosition = mul(float4(input.Position, 1.0f), gWorldMatrix);
output.Tangent = mul(normalize(input.Tangent), (float3x3)gWorldMatrix);
output.Normal = mul(normalize(input.Normal), (float3x3)gWorldMatrix);
return output;
}

//--------------------------------------
// Pixel Shader
//--------------------------------------
float4 PS(VS_OUTPUT input) : SV_TARGET
{

//Find the view direction
float3 viewDirection = normalize(input.WorldPosition.xyz - gViewInverseMatrix[3].xyz);

//Sample Diffuse Map
float3 sampledDiffuse = gDiffuseMap.Sample(samPoint, input.UV).rgb;

//Sample Normal Map
float3 normal = gNormalMap.Sample(samPoint, input.UV).rgb;
normal.rgb = (normal.rgb * 2.f) - 1.f;

//Transform Normal Map to Tangent Space
float3x3 tangentSpaceMatrix = {input.Tangent, cross(input.Normal, input.Tangent), input.Normal};
normal.rgb = mul(normal.rgb, tangentSpaceMatrix);

//Calculate Observed Area
float observedArea = dot(normal.rgb, -lightDirection);
observedArea = max(observedArea, 0.f);

//Sample Glossiness Map
float glossiness = gGlossinessMap.Sample(samPoint, input.UV).r;
glossiness *= shininess;

//Sample Specular Map
float specular = gSpecularMap.Sample(samPoint, input.UV).r;

//Calculate Reflected Light
float3 reflection = reflect(-lightDirection, normal.rgb);
float alfa = dot(reflection, viewDirection);
float PSR = 0.f;
if (alfa >= 0.f)
    PSR = specular * pow(alfa, glossiness);

//Calculate Phong Specular
float3 phong = { PSR, PSR, PSR };

//Calculate Final Color
float3 finalColor = (sampledDiffuse * kd) / PI;
finalColor = finalColor * lightIntensity * observedArea + phong;
return float4(finalColor, 1.f);
}

technique11 DefaultTechnique
{
   pass p0
	{
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}

// //--------------------------------------
// // Input/Output Structs
// //--------------------------------------
// struct VS_INPUT
// {
//     float3 Position : POSITION;
//     float3 Color : TEXCOORD;
// };

// struct VS_OUTPUT
// {
//     float4 Position : SV_POSITION;
//     float3 Color : TEXCOORD;
// };

// //--------------------------------------
// // Global Variables
// //--------------------------------------
// float4x4 gWorldViewProj : WorldViewProjection;
// Texture2D gDiffuseMap : DiffuseMap;

// SamplerState samPoint
// {
//     Filter = MIN_MAG_MIP_POINT;
//     AddressU = Wrap; // Or Mirror, Clamp, Border
//     AddressV = Wrap; // Or Mirror, Clamp, Border
// };


// //--------------------------------------
// // Vertex Shader
// //--------------------------------------

// VS_OUTPUT VS(VS_INPUT input)
// {
// VS_OUTPUT output = (VS_OUTPUT)0;
// //output.Position = float4(input.Position, 1.f);
// output.Position = mul(float4(input.Position, 1.f), gWorldViewProj);
// output.UV = input.UV;
// return output;
// }

// //--------------------------------------
// // Pixel Shader
// //--------------------------------------

// float4 PS(VS_OUTPUT input) : SV_TARGET
// {
//     return float4(input.UV, 1.f);
// }

// //--------------------------------------
// // Technique
// //--------------------------------------

// technique11 DefaultTechnique
// {
//     pass p0
//     {
//         SetVertexShader(CompileShader(vs_5_0, VS()));
//         SetGeometryShader( NULL );
//         SetPixelShader(CompileShader(ps_5_0, PS()));
//     }
// }
