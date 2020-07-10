#pragma once

#include <vector>

#include "windows.h"

class ComPort {
	DCB portSettings = {};
	HANDLE port = INVALID_HANDLE_VALUE;

	ComPort(ComPort const & other) = delete;
	operator=(ComPort const & other) = delete;
public:
	ComPort(std::string const & name, uint32_t rate = 115200) {
		this->port = CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (this->port == INVALID_HANDLE_VALUE) {
			throw std::runtime_error("Failed to create port");
		}

		memset(&portSettings, 0, sizeof(portSettings));
		portSettings.DCBlength = sizeof(portSettings);

		if (!BuildCommDCBA("baud=115200 data=8 parity=n stop=1 xon=off to=off odsr=off dtr=on rts=on", &portSettings)) {
			throw std::runtime_error("Failed to build COM port settings");
		}
		portSettings.BaudRate = rate;

		if (!SetCommState(this->port, &portSettings)) {
			throw std::runtime_error("Failed to set COM port settings");
		}
	}

	~ComPort() {
		CloseHandle(this->port);
	}

	void setBaudRate(uint32_t rate) {
		portSettings.BaudRate = rate;
		if (!SetCommState(this->port, &portSettings)) {
			throw std::runtime_error("Failed to set COM port settings");
		}
	}

	int pollData() {
		DWORD n = 0;
		uint8_t buf = 0;
		ReadFile(this->port, &buf, 1, (LPDWORD)((void *)&n), nullptr);
		return n == 0 ? -1 : buf;
	}

	bool sendData(uint8_t const * data, uintptr_t size) {
		int n = 0;
		return WriteFile(this->port, data, size, (LPDWORD)((void *)&n), nullptr);
	}

	uint8_t recvData() {
		auto const x = this->pollData();
		if (x < 0 || x > 255) {
			throw std::runtime_error("communication failed");
		}
		return uint8_t(x);
	}

	std::vector<uint8_t> recvBytes(int length) {
		std::vector<uint8_t> result(length);
		for (int i = 0; i < length; i++) {
			result[i] = recvData();
		}
		return result;
	}
};
