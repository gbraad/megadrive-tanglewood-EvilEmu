�,�        ��?�;       񟵦/       �q�          ion::render::Shader�Ln�         ?�50       񟵦$       �q�          nvidiacg.��         ?�5           .��      qD��      񟵦�      �q�       g  //Samplers
texture gIndexedTexture;
texture gPaletteTexture;

sampler2D gIndexedSampler = sampler_state
{
    Texture = <gIndexedTexture>;
	MinFilter = Point;
    MagFilter = Point;
};

sampler2D gPaletteSampler = sampler_state
{
    Texture = <gPaletteTexture>;
	MinFilter = Point;
    MagFilter = Point;
};

//Num colours in palette
float gPaletteSize = 16;

//Diffuse
float4 gDiffuseColour = float4(1.0, 1.0, 1.0, 1.0);

//Matrices
float4x4 gWorldViewProjectionMatrix : WorldViewProjection;

struct InputV
{
	float4 mPosition	: POSITION;
	float2 mTexCoord	: TEXCOORD0;
	float4 mColour		: COLOR;
};

struct OutputV
{
	float4 mPosition	: POSITION;
	float2 mTexCoord	: TEXCOORD0;
	float4 mColour		: COLOR;
};

OutputV VertexProgram(InputV input)
{
	OutputV output;

	output.mPosition = mul(gWorldViewProjectionMatrix, input.mPosition);
	output.mTexCoord = input.mTexCoord;
	output.mColour = gDiffuseColour;

	return output;
}

float4 FragmentProgram(OutputV input) : COLOR
{
	float4 data = tex2D(gIndexedSampler, input.mTexCoord);
	float index = data.r * 255.0;
	float4 colour = tex2D(gPaletteSampler, float2(index / gPaletteSize, 0.0));
	return (input.mColour * colour);
};

technique Main
{
    pass Pass0
	{
		VertexShader = compile vp40 VertexProgram();
		PixelShader = compile fp40 FragmentProgram();
    }
}
 ���5       񟵦)       �q�          VertexProgram?�5          .��      qD��      񟵦�      �q�       g  //Samplers
texture gIndexedTexture;
texture gPaletteTexture;

sampler2D gIndexedSampler = sampler_state
{
    Texture = <gIndexedTexture>;
	MinFilter = Point;
    MagFilter = Point;
};

sampler2D gPaletteSampler = sampler_state
{
    Texture = <gPaletteTexture>;
	MinFilter = Point;
    MagFilter = Point;
};

//Num colours in palette
float gPaletteSize = 16;

//Diffuse
float4 gDiffuseColour = float4(1.0, 1.0, 1.0, 1.0);

//Matrices
float4x4 gWorldViewProjectionMatrix : WorldViewProjection;

struct InputV
{
	float4 mPosition	: POSITION;
	float2 mTexCoord	: TEXCOORD0;
	float4 mColour		: COLOR;
};

struct OutputV
{
	float4 mPosition	: POSITION;
	float2 mTexCoord	: TEXCOORD0;
	float4 mColour		: COLOR;
};

OutputV VertexProgram(InputV input)
{
	OutputV output;

	output.mPosition = mul(gWorldViewProjectionMatrix, input.mPosition);
	output.mTexCoord = input.mTexCoord;
	output.mColour = gDiffuseColour;

	return output;
}

float4 FragmentProgram(OutputV input) : COLOR
{
	float4 data = tex2D(gIndexedSampler, input.mTexCoord);
	float index = data.r * 255.0;
	float4 colour = tex2D(gPaletteSampler, float2(index / gPaletteSize, 0.0));
	return (input.mColour * colour);
};

technique Main
{
    pass Pass0
	{
		VertexShader = compile vp40 VertexProgram();
		PixelShader = compile fp40 FragmentProgram();
    }
}
 ���7       񟵦+       �q�          FragmentProgram