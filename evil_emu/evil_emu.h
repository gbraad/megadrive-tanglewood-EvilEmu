////////////////////////////////////////////////////////////////////////////////
// EvilEmu
// Game framework using ion::engine with embedded SEGA Mega Drive ROM emulator
//
// Uses mega emulation code based on the work of Lee Hammerton and Jake Turner
//
// Matt Phillips
// http://www.bigevilcorporation.co.uk
////////////////////////////////////////////////////////////////////////////////

#include <ion/core/bootstrap/Application.h>
#include <ion/core/debug/Debug.h>
#include <ion/core/containers/FixedArray.h>
#include <ion/renderer/Texture.h>
#include <ion/renderer/Shader.h>
#include <ion/renderer/Material.h>
#include <ion/renderer/Primitive.h>

#include "settings.h"
#include "savegame.h"
#include "megaex/emulator.h"
#include "states/StateGame.h"
#include "states/StatePause.h"

class EvilEmu : public ion::framework::Application
{
public:
	EvilEmu();

	virtual bool Initialise();
	virtual void Shutdown();
	virtual bool Update(float deltaTime);
	virtual void Render();

	void ApplySettings();
	void ResetSettings();
	void LoadSettings();
	void SaveSettings();

	Settings m_settings;

private:
	bool InitialiseGameStates();
	void ShutdownGameStates();
	bool UpdateGameStates(float deltaTime);

	ion::io::ResourceHandle<ion::render::Texture> m_textureBackground;
	ion::render::Material* m_materialBackground;
	ion::render::Quad* m_quadPrimitiveBackground;

#if defined ION_RENDERER_SHADER
	ion::io::ResourceHandle<ion::render::Shader> m_shaderFlatTextured;
#endif

	//System
	void OnDisplayChanged(int displayIdx, const ion::Vector2i& resolution);

	//States
	ion::gamekit::StateManager m_stateManager;
	StateGame* m_stateGame;
	StatePause* m_statePause;

	//Save system
	SaveManager* m_saveManager;

	//Timing
	u64 m_frameCount;
	u64 m_startTicks;

	int m_currentdisplayIdx;
};