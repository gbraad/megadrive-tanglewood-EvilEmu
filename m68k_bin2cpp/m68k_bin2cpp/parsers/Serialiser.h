#pragma once

#include <iostream>

class Serialiser
{
public:
	enum class Endian
	{
		Little,
		Big
	};

	Serialiser(const uint8_t* ptr, Endian endian = Endian::Little)
	{
		m_ptr = ptr;
		m_start = ptr;
		m_endian = endian;
	}

	static void EndianSwap(int16_t& value)
	{
		int8_t* bytes = (int8_t*)&value;
		int8_t temp = bytes[0];
		bytes[0] = bytes[1];
		bytes[1] = temp;
	}

	static void EndianSwap(uint16_t& value)
	{
		int8_t* bytes = (int8_t*)&value;
		int8_t temp = bytes[0];
		bytes[0] = bytes[1];
		bytes[1] = temp;
	}

	static void EndianSwap(int32_t& value)
	{
		int8_t* bytes = (int8_t*)&value;
		int8_t temp = bytes[0];
		bytes[0] = bytes[3];
		bytes[3] = temp;
		temp = bytes[1];
		bytes[1] = bytes[2];
		bytes[2] = temp;
	}

	static void EndianSwap(uint32_t& value)
	{
		int8_t* bytes = (int8_t*)&value;
		int8_t temp = bytes[0];
		bytes[0] = bytes[3];
		bytes[3] = temp;
		temp = bytes[1];
		bytes[1] = bytes[2];
		bytes[2] = temp;
	}

	int64_t Seek(int64_t offset)
	{
		m_ptr = m_start + offset;
		return (int64_t)(m_ptr - m_start);
	}

	template <typename T> uint32_t Serialise(T& value)
	{
		const uint8_t* start = m_ptr;
		value.Serialise(*this);
		uintptr_t size = m_ptr - start;
		return (uint32_t)size;
	}

	uint32_t Serialise(int8_t& value)
	{
		value = *(int8_t*)m_ptr;
		m_ptr += sizeof(int8_t);
		return sizeof(int8_t);
	}

	uint32_t Serialise(uint8_t& value)
	{
		value = *(uint8_t*)m_ptr;
		m_ptr += sizeof(uint8_t);
		return sizeof(uint8_t);
	}

	uint32_t Serialise(uint16_t& value)
	{
		value = *(uint16_t*)m_ptr;
		m_ptr += sizeof(uint16_t);

		if (m_endian == Endian::Big)
			EndianSwap(value);

		return sizeof(uint16_t);
	}

	uint32_t Serialise(int16_t& value)
	{
		value = *(int16_t*)m_ptr;
		m_ptr += sizeof(int16_t);

		if (m_endian == Endian::Big)
			EndianSwap(value);

		return sizeof(int16_t);
	}

	uint32_t Serialise(uint32_t& value)
	{
		value = *(uint32_t*)m_ptr;
		m_ptr += sizeof(uint32_t);

		if (m_endian == Endian::Big)
			EndianSwap(value);

		return sizeof(uint32_t);
	}

	uint32_t Serialise(int32_t& value)
	{
		value = *(int32_t*)m_ptr;
		m_ptr += sizeof(int32_t);

		if (m_endian == Endian::Big)
			EndianSwap(value);

		return sizeof(int32_t);
	}

	uint32_t Serialise(std::string& value)
	{
		uint8_t length;
		Serialise(length);
		value.resize(length);
		memcpy(&value[0], m_ptr, length);
		m_ptr += length;
		return length;
	}

	uint32_t Serialise(std::string& value, uint32_t length)
	{
		value.resize(length);
		memcpy(&value[0], m_ptr, length);
		m_ptr += length;
		return length;
	}

	void Serialise(uint8_t* value, uint32_t length)
	{
		memcpy(value, m_ptr, length);
		m_ptr += length;
	}


private:
	const uint8_t* m_ptr;
	const uint8_t* m_start;
	Endian m_endian;
};