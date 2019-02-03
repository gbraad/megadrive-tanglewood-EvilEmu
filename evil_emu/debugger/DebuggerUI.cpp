#include "DebuggerUI.h"
#include "constants.h"

#include "megaex/cpu/m68k/cpu.h"
#include "megaex/memory.h"

#include <sstream>
#include <iomanip>

#include "roms/include_vars.h"

#define HEX2(val) std::hex << std::setfill('0') << std::setw(2) << std::uppercase << (int)val
#define HEX4(val) std::hex << std::setfill('0') << std::setw(4) << std::uppercase << (int)val
#define HEX8(val) std::hex << std::setfill('0') << std::setw(8) << std::uppercase << (int)val

DebuggerUI::DebuggerUI(ion::gui::GUI& gui, EmulatorThread& emulator, const ion::Vector2i& position, const ion::Vector2i& size)
	: ion::gui::Window("Debugger", position, size)
	, m_gui(gui)
	, m_emulator(emulator)
{
	for (int i = 0; i < 8; i++)
	{
		AddWidget(m_regs68k.dregs[i]);
		m_regs68k.aregs[i].SetArrangement(ion::gui::Widget::Arrangement::Horizontal);
		AddWidget(m_regs68k.aregs[i]);
	}

	AddWidget(m_regs68k.pc);
	AddWidget(m_regs68k.sr);

#if EVIL_EMU_GAME_TANGLEWOOD
	AddWidget(m_gameVars.password);
	AddWidget(m_gameVars.levelIdx);
	AddWidget(m_gameVars.levelAddr);
	AddWidget(m_gameVars.firefliesAct);
	AddWidget(m_gameVars.firefliesGame);
	AddWidget(m_gameVars.firefliesSave);
	AddWidget(m_gameVars.boulderDrops);
#endif
}

DebuggerUI::~DebuggerUI()
{

}

void DebuggerUI::Update(float deltaTime)
{
	Update68KRegs();
	UpdateGameVars();
	Window::Update(deltaTime);
}

void DebuggerUI::Update68KRegs()
{
	std::stringstream stream;

	m_emulator.m_renderCritSec.Begin();

	for (int i = 0; i < 8; i++)
	{
		stream.str("");
		stream << "d" << i << ": 0x" << HEX8(M68K::cpu_regs.D[i]);
		m_regs68k.dregs[i].SetText(stream.str());
	}

	for (int i = 0; i < 8; i++)
	{
		stream.str("");
		stream << "a" << i << ": 0x" << HEX8(M68K::cpu_regs.A[i]);
		m_regs68k.aregs[i].SetText(stream.str());
	}

	stream.str("");
	stream << "PC: 0x" << HEX8(M68K::cpu_regs.PC);
	m_regs68k.pc.SetText(stream.str());

	stream.str("");
	stream << "SR: 0x" << HEX4(M68K::cpu_regs.SR);
	m_regs68k.sr.SetText(stream.str());

	m_emulator.m_renderCritSec.End();
}

void DebuggerUI::UpdateGameVars()
{
#if EVIL_EMU_GAME_TANGLEWOOD
	std::stringstream stream;

	stream.str("");
	u32 password = MEM_getLong(snasm68k_symbol_CurrentSavePassword_val);
	stream << "Password: "
		<< std::string(1, (char)((password >> 0) & 0x0F) + 'A')
		<< std::string(1, (char)((password >> 4) & 0x0F) + 'A')
		<< std::string(1, (char)((password >> 8) & 0x0F) + 'A')
		<< std::string(1, (char)((password >> 12) & 0x0F) + 'A')
		<< std::string(1, (char)((password >> 16) & 0x0F) + 'A')
		<< std::string(1, (char)((password >> 20) & 0x0F) + 'A')
		<< std::string(1, (char)((password >> 24) & 0x0F) + 'A')
		<< std::string(1, (char)((password >> 28) & 0x0F) + 'A');
	m_gameVars.password.SetText(stream.str());

	stream.str("");
	u32 currentLevelAddr = MEM_getLong(snasm68k_symbol_CurrentLevel_val);
	int levelIdx = (currentLevelAddr > 0) ? (int)MEM_getByte(currentLevelAddr + snasm68k_symbol_Level_Index_val) : 0;
	stream << "Level index: " << levelIdx;
	m_gameVars.levelIdx.SetText(stream.str());

	stream.str("");
	stream << "Level addr: 0x" << HEX8(MEM_getLong(snasm68k_symbol_CurrentLevel_val));
	m_gameVars.levelAddr.SetText(stream.str());

	stream.str("");
	stream << "Fireflies (act): " << (int)MEM_getWord(snasm68k_symbol_FireflyPickupCountAct_val);
	m_gameVars.firefliesAct.SetText(stream.str());

	stream.str("");
	stream << "Fireflies (game): " << (int)MEM_getWord(snasm68k_symbol_FireflyPickupCountTotalUI_val);
	m_gameVars.firefliesGame.SetText(stream.str());

	stream.str("");
	stream << "Fireflies (last save): " << (int)MEM_getWord(snasm68k_symbol_FireflyPickupCountTotalSave_val);
	m_gameVars.firefliesSave.SetText(stream.str());

#endif
}
