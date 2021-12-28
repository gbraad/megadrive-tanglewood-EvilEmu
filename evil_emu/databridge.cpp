#include "databridge.h"
#include "megaex/memory.h"

void DataBridge::AddAddress(u32 address, int size, std::function<void(int address, int size, int value)> const& callback)
{
	Watch watch;
	watch.address = address;
	watch.size = size;
	watch.callback = callback;
	watch.cache = 0;
	m_watches.push_back(watch);
}

void DataBridge::Update()
{
	for (int i = 0; i < m_watches.size(); i++)
	{
		Watch& watch = m_watches[i];
		u32 value = 0;

		//Read value
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

		//If changed, issue callback
		if (value != watch.cache)
		{
			watch.callback(watch.address, watch.size, value);
		}

		//Callback can change value, so read back into the cache
		switch (watch.size)
		{
		case 1:
			watch.cache = MEM_getByte(watch.address);
			break;
		case 2:
			watch.cache = MEM_getWord(watch.address);
			break;
		case 4:
			watch.cache = MEM_getLong(watch.address);
			break;
		}
	}
}