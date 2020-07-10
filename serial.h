#include "com.h"

class serial {
	ComPort port;
	bool failed = false;

	int lookahead = -1;
public:
	serial()
		: port("COM3", 9600)
	{
		// ...
	}

	void print(char v) {
		if (!port.sendData(reinterpret_cast<uint8_t const *>(&v), 1)) {
			throw "failed to send data. checkme";
		}
	}

	void print(int v) {
		auto tosend = std::to_string(v);
		if (!port.sendData(reinterpret_cast<uint8_t const *>(tosend.data()), tosend.size())) {
			throw "failed to send number. checkme";
		}
	}

	void print(char const * v) {
		if (!port.sendData(reinterpret_cast<uint8_t const *>(v), strlen(v))) {
			throw "failed to send string. checkme";
		}
	}

	// совместим с ведруино, но всегда печатает 1 значение после точки
	void print(float v, int d) {
		print(int(v));
		print('.');
		print(int(v * 10.0) % 10);
	}

	uint8_t read() {
		if (lookahead != -1) {
			uint8_t result = lookahead & 0xff;
			lookahead = -1;
			return result;
		}
		return port.recvData();
	}

	float parseFloat() {
		int a = parseInt();
		uint8_t next = read();
		if (next != '.') {
			std::cout << "next is not dot, its a " << next << std::endl;
			lookahead = next;
			return a;
		}
		int b = parseInt();
		float result = b;
		while (result >= 1.0) {
			// std::cout << "fractional part is now " << result << std::endl;
			result /= 10.0; // xD, зато просто и работает
		}
		return result + a;
	}

	int parseInt() {
		int result = 0;
		while (true) {
			uint8_t next = read();
			if (next < '0' || next > '9') {
				lookahead = next;
				break;
			}
			result *= 10;
			result += int(next - '0'); // кек
		}
		return result;
	}
} Serial;
