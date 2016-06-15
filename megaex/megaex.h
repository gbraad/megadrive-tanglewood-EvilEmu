////////////////////////////////////////////////////////////////////////////////
// megaEx
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include <ion/core/bootstrap/Application.h>
#include <ion/core/debug/Debug.h>
#include <ion/renderer/Renderer.h>
#include <ion/renderer/Window.h>
#include <ion/renderer/Viewport.h>
#include <ion/renderer/Texture.h>
#include <ion/renderer/Shader.h>
#include <ion/renderer/Material.h>
#include <ion/renderer/Primitive.h>
#include <ion/renderer/Camera.h>
#include <ion/input/Keyboard.h>
#include <ion/input/Mouse.h>
#include <ion/input/Gamepad.h>

#include "emulator.h"

class MegaEx : public ion::framework::Application
{
public:
	MegaEx();

	virtual bool Initialise();
	virtual void Shutdown();
	virtual bool Update(float deltaTime);
	virtual void Render();

private:

	bool InitialiseRenderer();
	bool InitialiseInput();
	void ShutdownRenderer();
	void ShutdownInput();
	bool UpdateInput();

	void ChangeWindowSize(const ion::Vector2i& size);

	ion::render::Renderer* m_renderer;
	ion::render::Window* m_window;
	ion::render::Viewport* m_viewport;
	ion::render::Texture* m_renderTexture;
	ion::render::Shader* m_vertexShader;
	ion::render::Shader* m_pixelShader;
	ion::render::Material* m_material;
	ion::render::Quad* m_quadPrimitive;
	ion::render::Camera* m_camera;
	ion::input::Keyboard* m_keyboard;
	ion::input::Mouse* m_mouse;
	ion::input::Gamepad* m_gamepad;

	EmulatorState m_prevEmulatorState;

	static const ion::render::TexCoord s_texCoordsGame[4];
	static const ion::render::TexCoord s_texCoordsDebugger[4];
};