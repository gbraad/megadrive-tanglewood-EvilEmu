#include "DlgROMDisclaimer.h"

const char* DlgROMDisclaimer::s_disclaimerText =
	"DISCLAIMER\n\n" \
	"The EVIL_EMU Mega Drive ROM file and its content is protected\n" \
	"by both national and international copyright law.\n\n" \
	"Copying, reproduction, distribution, and modification of the software is\n" \
	"prohibited without express permission of Big Evil Corporation Ltd.\n\n" \
	"The EVIL_EMU ROM is included in the refund policy. If you are\n" \
	"refunded for your purchase of EVIL_EMU, you must discontinue use of\n" \
	"the ROM file and delete all copies.\n\n" \
	"EVIL_EMU is not licensed, sponsored, published, or endorsed by\n" \
	"SEGA Enterprises Ltd, SEGA Corporation, SEGA Holdings Co, or their affiliates.\n\n" \
	"Big Evil Corporation Ltd is an independent games development studio. This title\n" \
	"is an unofficial and unlicensed release for the SEGA Mega Drive console, and is not\n" \
	"affiliated with SEGA Enterprises Ltd, SEGA Corporation, or SEGA Holdings Co.\n\n" \
	"SEGA and MEGA DRIVE are trademarks of SEGA Holdings Co.";

DlgROMDisclaimer::DlgROMDisclaimer(ion::gui::GUI& gui, ion::gui::Font& font, std::function<void(const DialogBox&)> const& onClosed)
	: ion::gui::DialogBox("Mega Drive ROM", onClosed)
{
	m_result = false;

	m_txtDisclaimer = new ion::gui::TextBox(s_disclaimerText);
	m_btnDecline = new ion::gui::Button("Decline", std::bind(&DlgROMDisclaimer::OnButtonDecline, this, std::placeholders::_1));
	m_btnAgree = new ion::gui::Button("Agree", std::bind(&DlgROMDisclaimer::OnButtonAgree, this, std::placeholders::_1));

	m_btnAgree->SetArrangement(ion::gui::Widget::Arrangement::Horizontal);

	AddWidget(*m_txtDisclaimer);
	AddWidget(*m_btnDecline);
	AddWidget(*m_btnAgree);
}

DlgROMDisclaimer::~DlgROMDisclaimer()
{

}

bool DlgROMDisclaimer::Agreed() const
{
	return m_result;
}

void DlgROMDisclaimer::OnButtonAgree(const ion::gui::Button& button)
{
	Close();
	m_result = true;
}

void DlgROMDisclaimer::OnButtonDecline(const ion::gui::Button& button)
{
	Close();
	m_result = false;
}