////////////////////////////////////////////////////////////////////////////////
// megaEx
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include "megaex.h"
#include "config.h"

static const int g_top = 128;
static const int g_bottom = 128 + 224;
static const int g_left = 128;
static const int g_right = 128 + (40 * 8);
static const float g_borderTop = (1.0f / HEIGHT) * (float)g_top;
static const float g_borderBottom = (1.0f / HEIGHT) * (float)(HEIGHT - g_bottom);
static const float g_borderLeft = (1.0f / WIDTH) * (float)g_left;
static const float g_borderRight = (1.0f / WIDTH) * (float)(WIDTH - g_right);

const ion::render::TexCoord MegaEx::s_texCoordsGame[4] =
{
	ion::Vector2(g_borderLeft, g_borderTop),
	ion::Vector2(g_borderLeft, 1.0f - g_borderBottom),
	ion::Vector2(1.0f - g_borderRight, 1.0f - g_borderBottom),
	ion::Vector2(1.0f - g_borderRight, g_borderTop)
};

const ion::render::TexCoord MegaEx::s_texCoordsDebugger[4] =
{
	ion::Vector2(0.0f, 0.0f),
	ion::Vector2(0.0f, 1.0f),
	ion::Vector2(1.0f, 1.0f),
	ion::Vector2(1.0f, 0.0f)
};

MegaEx::MegaEx() : ion::framework::Application("megaEx")
{
	m_renderer = NULL;
	m_window = NULL;
	m_viewport = NULL;
	m_renderTexture = NULL;
	m_vertexShader = NULL;
	m_pixelShader = NULL;
	m_material = NULL;
	m_quadPrimitive = NULL;
	m_camera = NULL;
	m_keyboard = NULL;
	m_mouse = NULL;
	m_gamepad = NULL;

	m_prevEmulatorState = eState_Running;
}

bool MegaEx::Initialise()
{
	if(!InitialiseRenderer())
	{
		return false;
	}

	if(!InitialiseInput())
	{
		return false;
	}

	if(!InitialiseEmulator("ROMS\\TANGLEWD.BIN"))
	{
		return false;
	}

	//Set window title
	m_window->SetTitle(EmulatorGetROMTitle());

	//ChangeWindowSize(ion::Vector2i(m_viewport->GetWidth() * 2, m_viewport->GetHeight() * 2));

	return true;
}

void MegaEx::Shutdown()
{
	ShutdownInput();
	ShutdownRenderer();
}

bool MegaEx::Update(float deltaTime)
{
	//Update input
	bool inputQuit = !UpdateInput();

	//Update window
	bool windowQuit = !m_window->Update();

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
	m_renderTexture->SetPixels(ion::render::Texture::eBGRA, videoMemory);

	return !windowQuit && !inputQuit;
}

void MegaEx::Render()
{
	m_renderer->BeginFrame(*m_viewport, m_window->GetDeviceContext());
	m_renderer->ClearColour();
	m_renderer->ClearDepth();

	//Bind material and draw quad
	m_material->Bind(ion::Matrix4(), m_camera->GetTransform().GetInverse(), m_renderer->GetProjectionMatrix());
	m_renderer->DrawVertexBuffer(m_quadPrimitive->GetVertexBuffer(), m_quadPrimitive->GetIndexBuffer());
	m_material->Unbind();

	m_renderer->SwapBuffers();
	m_renderer->EndFrame();
}

bool MegaEx::InitialiseRenderer()
{
	m_window = ion::render::Window::Create("megaEx", DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, false);
	m_renderer = ion::render::Renderer::Create(m_window->GetDeviceContext());
	m_viewport = new ion::render::Viewport(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, ion::render::Viewport::eOrtho2DAbsolute);
	m_renderTexture = ion::render::Texture::Create(WIDTH, HEIGHT, ion::render::Texture::eRGB, ion::render::Texture::eRGB, ion::render::Texture::eBPP24, false, NULL);
	m_vertexShader = ion::render::Shader::Create();
	m_pixelShader = ion::render::Shader::Create();
	m_material = new ion::render::Material();
	m_quadPrimitive = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2(DEFAULT_SCREEN_WIDTH / 2, DEFAULT_SCREEN_HEIGHT / 2));
	m_camera = new ion::render::Camera();

	m_quadPrimitive->SetTexCoords(s_texCoordsGame);
	m_viewport->SetClearColour(ion::Colour(1.0f, 0.0f, 0.0f, 1.0f));
	m_camera->SetPosition(ion::Vector3(-DEFAULT_SCREEN_WIDTH / 2.0f, -DEFAULT_SCREEN_HEIGHT / 2.0f, 0.1f));

	//Load shaders
	if(!m_vertexShader->Load("shaders/flattextured_v.ion.shader"))
	{
		ion::debug::Error("Failed to load vertex shader\n");
		return false;
	}

	if(!m_pixelShader->Load("shaders/flattextured_p.ion.shader"))
	{
		ion::debug::Error("Failed to load pixel shader\n");
		return false;
	}

	//Setup material
	m_material->AddDiffuseMap(m_renderTexture);
	m_material->SetDiffuseColour(ion::Colour(1.0f, 1.0f, 1.0f));
	m_material->SetVertexShader(m_vertexShader);
	m_material->SetPixelShader(m_pixelShader);

	//Setup texture filtering
	m_renderTexture->SetMinifyFilter(ion::render::Texture::eFilterNearest);
	m_renderTexture->SetMagnifyFilter(ion::render::Texture::eFilterNearest);
	m_renderTexture->SetWrapping(ion::render::Texture::eWrapClamp);

	return true;
}

void MegaEx::ShutdownRenderer()
{
	if(m_camera)
		delete m_camera;

	if(m_quadPrimitive)
		delete m_quadPrimitive;

	if(m_material)
		delete m_material;

	if(m_pixelShader)
		delete m_pixelShader;

	if(m_vertexShader)
		delete m_vertexShader;

	if(m_renderTexture)
		delete m_renderTexture;

	if(m_viewport)
		delete m_viewport;

	if(m_renderer)
		delete m_renderer;

	if(m_window)
		delete m_window;
}

bool MegaEx::InitialiseInput()
{
	m_keyboard = new ion::input::Keyboard();
	m_mouse = new ion::input::Mouse();
	m_gamepad = new ion::input::Gamepad();
	return true;
}

void MegaEx::ShutdownInput()
{
	if(m_gamepad)
		delete m_gamepad;

	if(m_mouse)
		delete m_mouse;

	if(m_keyboard)
		delete m_keyboard;
}

bool MegaEx::UpdateInput()
{
	m_keyboard->Update();
	m_mouse->Update();
	m_gamepad->Update();

	if(m_keyboard->KeyDown(DIK_UP))
		EmulatorButtonDown(eBtn_Up);
	else
		EmulatorButtonUp(eBtn_Up);

	if(m_keyboard->KeyDown(DIK_DOWN))
		EmulatorButtonDown(eBtn_Down);
	else
		EmulatorButtonUp(eBtn_Down);

	if(m_keyboard->KeyDown(DIK_LEFT))
		EmulatorButtonDown(eBtn_Left);
	else
		EmulatorButtonUp(eBtn_Left);

	if(m_keyboard->KeyDown(DIK_RIGHT))
		EmulatorButtonDown(eBtn_Right);
	else
		EmulatorButtonUp(eBtn_Right);

	if(m_keyboard->KeyDown(DIK_RETURN))
		EmulatorButtonDown(eBtn_Start);
	else
		EmulatorButtonUp(eBtn_Start);

	if(m_keyboard->KeyDown(DIK_D))
		EmulatorButtonDown(eBtn_A);
	else
		EmulatorButtonUp(eBtn_A);

	if(m_keyboard->KeyDown(DIK_S))
		EmulatorButtonDown(eBtn_B);
	else
		EmulatorButtonUp(eBtn_B);

	if(m_keyboard->KeyDown(DIK_A))
		EmulatorButtonDown(eBtn_C);
	else
		EmulatorButtonUp(eBtn_C);

	return !m_keyboard->KeyDown(DIK_ESCAPE);
}

void MegaEx::ChangeWindowSize(const ion::Vector2i& size)
{
	m_window->Resize(size.x, size.y);
	m_viewport->Resize(size.x, size.y);
	m_camera->SetPosition(ion::Vector3(-size.x / 2.0f, -size.y / 2.0f, 0.1f));
	m_renderer->OnResize(size.x, size.y);

	//Recreate quad
	delete m_quadPrimitive;
	m_quadPrimitive = new ion::render::Quad(ion::render::Quad::xy, ion::Vector2(size.x / 2, size.y / 2));
	m_quadPrimitive->SetTexCoords(s_texCoordsGame);
}