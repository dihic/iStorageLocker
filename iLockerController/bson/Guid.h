#ifndef _GUID_H
#define _GUID_H

#include <cstdint>
#include <cstring>
#include <string>

#define GUID_VERSION 4
#define GUID_BIGENDIAN

class Guid
{
	private:
		static const Guid Empty;
		static std::string DigitToStringHelper(std::uint8_t *digit, std::int8_t size, bool bigEndian);
		std::uint8_t raw[16]  __attribute__((aligned(4)));
		std::string str;
	public:
		Guid() { std::memset(raw, 0, 16); }
		Guid(const std::uint8_t *data);
		Guid(const Guid &g) {	std::memcpy(raw, g.raw, 16); }
		~Guid() { }
		void New();
		void Zero() { std::memset(raw, 0, 16); }
		bool operator == (Guid& _test);
		bool operator != (Guid& _test);
		//static Guid &Empty() { return empty; }
		const std::string &ToString();
		std::uint8_t *ToByteArray() { return raw; }
};

#endif
