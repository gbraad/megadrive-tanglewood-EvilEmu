#pragma once

#include <ion/gui/GUI.h>
#include <ion/gui/DialogBox.h>
#include <ion/gui/Font.h>
#include <ion/gui/Button.h>
#include <ion/gui/TextBox.h>

class DlgROMDisclaimer : public ion::gui::DialogBox
{
public:
	DlgROMDisclaimer(ion::gui::GUI& gui, ion::gui::Font& font, std::function<void(const DialogBox&)> const& onClosed);
	~DlgROMDisclaimer();

	bool Agreed() const;

private:
	void OnButtonAgree(const ion::gui::Button& button);
	void OnButtonDecline(const ion::gui::Button& button);

	ion::gui::TextBox* m_txtDisclaimer;
	ion::gui::Button* m_btnDecline;
	ion::gui::Button* m_btnAgree;

	bool m_result;

	static const char* s_disclaimerText;
};