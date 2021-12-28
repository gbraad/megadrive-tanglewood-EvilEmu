#pragma once

#include <ion/core/Types.h>

#include <functional>
#include <vector>

class DataBridge
{
public:
	void AddAddress(u32 address, int size, std::function<void(int address, int size, int value)> const& callback);

	void Update();

private:

	struct Watch
	{
		u32 address;
		int size;
		u32 cache;
		std::function<void(int address, int size, int value)> callback;
	};

	std::vector<Watch> m_watches;
};
