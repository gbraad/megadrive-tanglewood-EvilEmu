�,�        ��?�;       񟵦/       �q�          ion::render::Shader�Ln�]	         ?�5,       񟵦        �q�          glsl.��         ?�5           .��      qD�I      񟵦=      �q�       !  #version 330 core

//Diffuse
uniform vec4 gDiffuseColour = vec4(1.0f, 1.0f, 1.0f, 1.0f);

//Matrices
uniform mat4 gWorldViewProjectionMatrix;

//Vertex input
//Matches ion::render::VertexBuffer::ElementType
layout (location = 0) in vec4 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec4 in_colour;
layout (location = 3) in vec2 in_texCoord;

//Vertex output
out vec4 colour;

void main()
{
	gl_Position = gWorldViewProjectionMatrix * in_position;
	colour = in_colour * gDiffuseColour;
}
 ���,       񟵦        �q�          main?�5          .��       qD��       񟵦�       �q�       �   #version 330 core

//Fragment input
in vec4 colour;

//Fragment output
out vec4 outputColour;

void main()
{
	outputColour = colour;
}
 ���,       񟵦        �q�          main?�50       񟵦$       �q�          nvidiacg.�L         ?�5           .��      qD�L      񟵦@      �q�       $  
//Diffuse
float4 gDiffuseColour = float4(1.0f, 1.0f, 1.0f, 1.0f);

//Matrices
float4x4 gWorldViewProjectionMatrix;

struct InputV
{
	float4 position	: POSITION;
	float4 colour	: COLOR;
};

struct OutputV
{
	float4 position	: POSITION;
	float4 colour	: COLOR;
};

OutputV VertexProgram(InputV input)
{
	OutputV output;

	output.position = mul(gWorldViewProjectionMatrix, input.position);
	output.colour = gDiffuseColour;

	return output;
}

float4 FragmentProgram(OutputV input) : COLOR
{
	return input.colour;
};
 ���5       񟵦)       �q�          VertexProgram?�5          .��      qD�L      񟵦@      �q�       $  
//Diffuse
float4 gDiffuseColour = float4(1.0f, 1.0f, 1.0f, 1.0f);

//Matrices
float4x4 gWorldViewProjectionMatrix;

struct InputV
{
	float4 position	: POSITION;
	float4 colour	: COLOR;
};

struct OutputV
{
	float4 position	: POSITION;
	float4 colour	: COLOR;
};

OutputV VertexProgram(InputV input)
{
	OutputV output;

	output.position = mul(gWorldViewProjectionMatrix, input.position);
	output.colour = gDiffuseColour;

	return output;
}

float4 FragmentProgram(OutputV input) : COLOR
{
	return input.colour;
};
 ���7       񟵦+       �q�          FragmentProgram