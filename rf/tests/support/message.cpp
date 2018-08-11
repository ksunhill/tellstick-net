#include <string>
#include <sstream>
#include <stdio.h>

extern "C" {

static std::stringstream buffer;

void rfMessageBeginRaw() {
	buffer.str(std::string());
	buffer << "+RAW" << std::hex << std::uppercase;
}

void rfMessageEnd(unsigned char type) {}

void rfMessageAddLong(const char *key, unsigned long value) {
	buffer << key << ":" << value << ";";
}

void rfMessageAddString(const char *key, const char *value) {
	buffer << key << ":" << value << ";";
}

void rfMessageAddHexString(const char *key, const char *value, const unsigned char length) {
	buffer << key << ":";
	for(unsigned char i = 0; i < length; ++i) {
		buffer << (int)(value[i] >> 4 & 0xF) << (int)(value[i] & 0xF);
	}
	buffer << ";";
}

}

std::string getBuffer() {
	return buffer.str();
}
