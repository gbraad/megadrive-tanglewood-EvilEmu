////////////////////////////////////////////////////////////////////////////////
// megaEx
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include <ion/renderer/Renderer.h>
#include <ion/renderer/Camera.h>
#include <ion/input/Keyboard.h>
#include <ion/input/Mouse.h>
#include <ion/input/Gamepad.h>

#include <ion/core/debug/Debug.h>

#include "StateGame.h"

#include <GL/gl.h>

static const float g_borderTop = 0.0f;
static const float g_borderBottom = 1.0f - ((float)VDP_SCREEN_HEIGHT / (float)RENDER_TEXTURE_HEIGHT);
static const float g_borderLeft = 0.0f;
static const float g_borderRight = 1.0f - ((float)VDP_SCREEN_WIDTH / (float)RENDER_TEXTURE_WIDTH);

u32 display[RENDER_TEXTURE_WIDTH * RENDER_TEXTURE_HEIGHT];

const ion::render::TexCoord StateGame::s_texCoordsGame[4] =
{
	ion::Vector2(g_borderLeft, g_borderTop),
	ion::Vector2(g_borderLeft, 1.0f - g_borderBottom),
	ion::Vector2(1.0f - g_borderRight, 1.0f - g_borderBottom),
	ion::Vector2(1.0f - g_borderRight, g_borderTop)
};

const ion::render::TexCoord StateGame::s_texCoordsDebugger[4] =
{
	ion::Vector2(0.0f, 0.0f),
	ion::Vector2(0.0f, 1.0f),
	ion::Vector2(1.0f, 1.0f),
	ion::Vector2(1.0f, 0.0f)
};

StateGame::StateGame(ion::gamekit::StateManager& stateManager, ion::io::ResourceManager& resourceManager, const ion::Vector2i& windowSize, const ion::Vector2i& emulatorSize)
	: ion::gamekit::State(stateManager, resourceManager)
	, m_windowSize(windowSize)
	, m_emulatorSize(emulatorSize)
{
	m_renderTexture = NULL;

	m_tickCount = 0;
	m_renderCount = 0;

#if defined ION_RENDERER_SHADER
	m_vertexShader = NULL;
	m_pixelShader = NULL;
#endif

	m_material = NULL;
	m_quadPrimitive = NULL;

	m_prevEmulatorState = eState_Running;
}

void StateGame::OnEnterState()
{
	m_renderTexture = ion::render::Texture::Create(RENDER_TEXTURE_WIDTH, RENDER_TEXTURE_HEIGHT);

#if defined ION_RENDERER_SHADER
	m_vertexShader = ion::render::Shader::Create();
	m_pixelShader = ion::render::Shader::Create();
#endif

	m_material = new ion::render::Material();
	m_quadPrimitive = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2(m_windowSize.x / 2.0f, m_windowSize.y / 2.0f));
	m_cubePrimitive = new ion::render::Box(ion::Vector3(1.0f, 1.0f, 1.0f));

#if defined ION_RENDERER_SHADER
	//Load shaders
	if(!m_vertexShader->Load("shaders/flattextured_v.ion.shader"))
	{
		ion::debug::Error("Failed to load vertex shader\n");
	}

	if(!m_pixelShader->Load("shaders/flattextured_p.ion.shader"))
	{
		ion::debug::Error("Failed to load pixel shader\n");
	}
#endif

	//Setup material
	m_material->AddDiffuseMap(m_renderTexture);
	m_material->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));

#if defined ION_RENDERER_SHADER
	m_material->SetVertexShader(m_vertexShader);
	m_material->SetPixelShader(m_pixelShader);
#endif

	//Setup texture filtering
	m_renderTexture->SetMinifyFilter(ion::render::Texture::eFilterNearest);
	m_renderTexture->SetMagnifyFilter(ion::render::Texture::eFilterNearest);
	m_renderTexture->SetWrapping(ion::render::Texture::eWrapClamp);

	//Set default coords
	m_quadPrimitive->SetTexCoords(s_texCoordsGame);
}

void StateGame::OnLeaveState()
{
	if(m_quadPrimitive)
		delete m_quadPrimitive;

	if(m_material)
		delete m_material;

#if defined ION_RENDERER_SHADER
	if(m_pixelShader)
		delete m_pixelShader;

	if(m_vertexShader)
		delete m_vertexShader;
#endif

	if(m_renderTexture)
		delete m_renderTexture;
}

void StateGame::OnPauseState()
{

}

void StateGame::OnResumeState()
{

}

void StateGame::Update(float deltaTime, ion::input::Keyboard* keyboard, ion::input::Mouse* mouse, ion::input::Gamepad* gamepad)
{
	//Update emulator
	EmulatorState emulatorState = TickEmulator();
	
	if(emulatorState != m_prevEmulatorState)
	{
		//Emulator switched from running to debugger or back, change rendering mode
		if(emulatorState == eState_Running)
		{
			m_quadPrimitive->SetTexCoords(s_texCoordsGame);
		}
		else if(emulatorState = eState_Debugger)
		{
			m_quadPrimitive->SetTexCoords(s_texCoordsDebugger);
		}
	
		m_prevEmulatorState = emulatorState;
	}

	//Copy output pixels to render texture
	for(int y = 0; y < VDP_SCREEN_HEIGHT; y++)
	{
		memcpy(display + (RENDER_TEXTURE_WIDTH * y), (u32*)videoMemory + (WIDTH * (y + 128)) + 128, VDP_SCREEN_WIDTH * sizeof(u32));
	}

	m_renderTexture->Load(RENDER_TEXTURE_WIDTH, RENDER_TEXTURE_HEIGHT, ion::render::Texture::eRGBA, ion::render::Texture::eRGBA, ion::render::Texture::eBPP24, false, (u8*)display);

	m_tickCount++;
}

void StateGame::Render(ion::render::Renderer& renderer, ion::render::Camera& camera)
{
	if(m_tickCount > m_renderCount)
	{
		//Bind material and draw quad
		m_material->Bind(ion::Matrix4(), camera.GetTransform().GetInverse(), renderer.GetProjectionMatrix());

#if defined ION_RENDERER_FIXED
		renderer.SetMatrix(camera.GetTransform().GetInverse());
#endif

		renderer.SetFaceCulling(ion::render::Renderer::eNoCull);

		renderer.DrawVertexBuffer(m_quadPrimitive->GetVertexBuffer(), m_quadPrimitive->GetIndexBuffer());

		m_material->Unbind();

		m_renderCount++;
	}
}