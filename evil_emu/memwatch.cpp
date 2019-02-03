#include "memwatch.h"
#include "megaex/memory.h"

void MemWatcher::AddAddress(u32 address, int size, std::function<void(int address, int size, int value)> const& callback)
{
	Watch watch;
	watch.address = address;
	watch.size = size;
	watch.callback = callback;
	watch.cache = 0;
	m_watches.push_back(watch);
}

void MemWatcher::Update()
{
	for (int i = 0; i < m_watches.size(); i++)
	{
		Watch& watch = m_watches[i];
		u32 value = 0;

		switch (watch.size)
		{
		case 1:
			value = MEM_getByte(watch.address);
			break;
		case 2:
			value = MEM_getWord(watch.address);
			break;
		case 4:
			value = MEM_getLong(watch.address);
			break;
		}

		if (value != watch.cache)
		{
			watch.cache = value;
			watch.callback(watch.address, watch.size, value);
		}
	}
}