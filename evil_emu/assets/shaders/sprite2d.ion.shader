�,�        ��?�;       񟵦/       �q�          ion::render::Shader�Ln��         ?�50       񟵦$       �q�          nvidiacg.�l         ?�5           .�      qD��      񟵦�      �q�       �  
//Samplers
sampler2D gSpriteSheet;

//Lighting
float4 gDiffuseColour = float4(1.0f, 1.0f, 1.0f, 1.0f);

//Matrices
float4x4 gWorldViewProjectionMatrix;

//Sprite grid
float2 gSpriteSheetGridSize = float2(1.0f, 1.0f);

//Current sprite frame
float gCurrentFrame = 0.0f;

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

	//Set position and colour
	output.mPosition = mul(gWorldViewProjectionMatrix, input.mPosition);
	output.mColour = gDiffuseColour;

	//Get sprite sheet cell coordinates
	float2 cellCoords;
	cellCoords.x = mod(gCurrentFrame, gSpriteSheetGridSize.x);
	cellCoords.y = mod(floor((gCurrentFrame) / gSpriteSheetGridSize.x), gSpriteSheetGridSize.y);

	//Get top left/bottom right bounds
	float2 cellTopLeft = (1.0f / gSpriteSheetGridSize) * cellCoords;
	float2 cellBottomRight = (1.0f / gSpriteSheetGridSize) * (cellCoords + 1.0f);

	//Lerp between bounds by input coord
	output.mTexCoord.x = lerp(cellTopLeft.x, cellBottomRight.x, input.mTexCoord.x);
	output.mTexCoord.y = lerp(cellTopLeft.y, cellBottomRight.y, input.mTexCoord.y);

	return output;
}

float4 FragmentProgram(OutputV input) : COLOR
{
	return input.mColour * tex2D(gSpriteSheet, input.mTexCoord);
};
 ���5       񟵦)       �q�          VertexProgram?�5          .�      qD��      񟵦�      �q�       �  
//Samplers
sampler2D gSpriteSheet;

//Lighting
float4 gDiffuseColour = float4(1.0f, 1.0f, 1.0f, 1.0f);

//Matrices
float4x4 gWorldViewProjectionMatrix;

//Sprite grid
float2 gSpriteSheetGridSize = float2(1.0f, 1.0f);

//Current sprite frame
float gCurrentFrame = 0.0f;

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

	//Set position and colour
	output.mPosition = mul(gWorldViewProjectionMatrix, input.mPosition);
	output.mColour = gDiffuseColour;

	//Get sprite sheet cell coordinates
	float2 cellCoords;
	cellCoords.x = mod(gCurrentFrame, gSpriteSheetGridSize.x);
	cellCoords.y = mod(floor((gCurrentFrame) / gSpriteSheetGridSize.x), gSpriteSheetGridSize.y);

	//Get top left/bottom right bounds
	float2 cellTopLeft = (1.0f / gSpriteSheetGridSize) * cellCoords;
	float2 cellBottomRight = (1.0f / gSpriteSheetGridSize) * (cellCoords + 1.0f);

	//Lerp between bounds by input coord
	output.mTexCoord.x = lerp(cellTopLeft.x, cellBottomRight.x, input.mTexCoord.x);
	output.mTexCoord.y = lerp(cellTopLeft.y, cellBottomRight.y, input.mTexCoord.y);

	return output;
}

float4 FragmentProgram(OutputV input) : COLOR
{
	return input.mColour * tex2D(gSpriteSheet, input.mTexCoord);
};
 ���7       񟵦+       �q�          FragmentProgram