
#include "MenuManual.h"
#include "MenuCommon.h"

#include <ion/core/utils/STL.h>
#include <ion/engine/Engine.h>

#include <sstream>

#define MANUAL_WINDOW_BORDER	100
#define MANUAL_BUTTON_SPACE		100
#define MANUAL_NUM_PAGES		20

MenuManual::MenuManual(ion::gui::GUI& gui, Settings& settings, const ion::Vector2i& position, const ion::Vector2i& size)
	: ion::gui::Window("Game Manual", position, size)
	, m_gui(gui)
	, m_settings(settings)
{
	SetCentred(true);
	SetBackgroundAlpha(0.8f);
	AllowMove(false);
	AllowResize(false);
	AllowRollUp(false);

	m_currentImageIdx = 0;

	for (int i = 1; i < MANUAL_NUM_PAGES + 1; i++)
	{
		std::string filename = "textures/manual/" + std::to_string(i);
		m_textures.push_back(ion::engine.io.resourceManager->GetResource<ion::render::Texture>(filename));
	}

	m_currentTexture = m_textures[0];

	//Add widgets
	m_image = new ion::gui::Image(m_currentTexture);
	m_buttonNext = new ion::gui::Button("Next", std::bind(&MenuManual::OnButtonNext, this, std::placeholders::_1));
	m_buttonPrev = new ion::gui::Button("Prev", std::bind(&MenuManual::OnButtonPrev, this, std::placeholders::_1));
	m_buttonBack = new ion::gui::Button("Back", std::bind(&MenuManual::OnButtonBack, this, std::placeholders::_1));

	m_buttonNext->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
	m_buttonPrev->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));
	m_buttonBack->SetSize(ion::Vector2i(MENU_BUTTON_WIDTH, 0));

	m_buttonNext->SetArrangement(ion::gui::Widget::Arrangement::Horizontal);

	AddWidget(*m_image);
	AddWidget(*m_buttonPrev);
	AddWidget(*m_buttonNext);
	AddWidget(*m_buttonBack);

	SyncSettings();
}

MenuManual::~MenuManual()
{
	delete m_buttonNext;
	delete m_buttonPrev;
	delete m_buttonBack;
}

void MenuManual::SyncSettings()
{
	if (m_currentTexture)
	{
		ion::Vector2i resolution(ion::engine.render.window->GetClientAreaWidth(), ion::engine.render.window->GetClientAreaHeight());

		//Make sure there's enough room for buttons in wide resolutions
		float imageAspect = ((float)m_currentTexture->GetHeight() / (float)m_currentTexture->GetWidth());
		int windowWidth = resolution.x - MANUAL_WINDOW_BORDER;
		int windowHeight = (int)((float)windowWidth * imageAspect) + MANUAL_BUTTON_SPACE;

		if (windowHeight > resolution.y)
		{
			windowWidth = (int)((float)(m_settings.resolution.y - MANUAL_WINDOW_BORDER - MANUAL_BUTTON_SPACE) / imageAspect);
		}

		m_image->SetSize(ion::Vector2i(windowWidth - (m_image->GetImageBorder().x * 2), 0));
		//SetSize(ion::Vector2i(windowWidth, 0));
	}
}

void MenuManual::OnButtonNext(const ion::gui::Button& button)
{
	m_currentImageIdx = (m_currentImageIdx + 1) % m_textures.size();
	m_currentTexture = m_textures[m_currentImageIdx];
}

void MenuManual::OnButtonPrev(const ion::gui::Button& button)
{
	m_currentImageIdx--;
	if (m_currentImageIdx < 0)
		m_currentImageIdx = (int)m_textures.size() - 1;

	m_currentTexture = m_currentTexture = m_textures[m_currentImageIdx];
	m_image->SetTexture(m_currentTexture);
}

void MenuManual::OnButtonBack(const ion::gui::Button& button)
{
	m_gui.PopWindow();
}