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

#include "StateGame.h"

static const int g_top = 128;
static const int g_bottom = 128 + 224;
static const int g_left = 128;
static const int g_right = 128 + (40 * 8);
static const float g_borderTop = (1.0f / HEIGHT) * (float)g_top;
static const float g_borderBottom = (1.0f / HEIGHT) * (float)(HEIGHT - g_bottom);
static const float g_borderLeft = (1.0f / WIDTH) * (float)g_left;
static const float g_borderRight = (1.0f / WIDTH) * (float)(WIDTH - g_right);

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
	m_material = NULL;
	m_quadPrimitive = NULL;

#if defined ION_RENDERER_SHADER
	m_vertexShader = NULL;
	m_pixelShader = NULL;
#endif

	m_emulatorThread = NULL;
	m_prevEmulatorState = eState_Running;
}

void StateGame::OnEnterState()
{
	m_renderTexture = ion::render::Texture::Create(m_emulatorSize.x, m_emulatorSize.y, ion::render::Texture::eBGRA, ion::render::Texture::eBGRA, ion::render::Texture::eBPP24, false, true, NULL);
	m_material = new ion::render::Material();
	m_quadPrimitive = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2(m_windowSize.x / 2.0f, m_windowSize.y / 2.0f));

	//Setup material
	m_material->AddDiffuseMap(m_renderTexture);
	m_material->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));

#if defined ION_RENDERER_SHADER
	//Create shaders
	m_vertexShader = ion::render::Shader::Create();
	m_pixelShader = ion::render::Shader::Create();

	//Load shaders
	if(!m_vertexShader->Load("shaders/flattextured_v.ion.shader"))
	{
		ion::debug::Error("Failed to load vertex shader\n");
	}

	if(!m_pixelShader->Load("shaders/flattextured_p.ion.shader"))
	{
		ion::debug::Error("Failed to load pixel shader\n");
	}

	m_material->SetVertexShader(m_vertexShader);
	m_material->SetPixelShader(m_pixelShader);
#endif

	//Setup texture filtering
	m_renderTexture->SetMinifyFilter(ion::render::Texture::eFilterNearest);
	m_renderTexture->SetMagnifyFilter(ion::render::Texture::eFilterNearest);
	m_renderTexture->SetWrapping(ion::render::Texture::eWrapClamp);

	//Set default coords
	m_quadPrimitive->SetTexCoords(s_texCoordsGame);

	//Create and run emulator thread
	m_emulatorThread = new EmulatorThread();
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
#if !EMU_THREADED
	m_emulatorThread->TickEmulator(deltaTime);
#endif

#if 0 // TODO
	//Get emulator state

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
#endif

	//Copy output pixels to render texture
	m_emulatorThread->m_renderCritSec.Begin();
	m_renderTexture->SetPixels(ion::render::Texture::eBGRA, videoMemory);
	m_emulatorThread->m_renderCritSec.End();
}

void StateGame::Render(ion::render::Renderer& renderer, ion::render::Camera& camera)
{
	//Bind material and draw quad
	m_material->Bind(ion::Matrix4(), camera.GetTransform().GetInverse(), renderer.GetProjectionMatrix());
	renderer.DrawVertexBuffer(m_quadPrimitive->GetVertexBuffer(), m_quadPrimitive->GetIndexBuffer());
	m_material->Unbind();
}
