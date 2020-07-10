class MKS_902B {
public:
	enum Address : uint8_t {
		Invalid   = 0x00,
		Any       = 0xFE,
		AnySilent = 0xFF,
	};

	enum class Result : int {
		Success    = 0x00,
		Failed     = 0x01,
		Internal   = 0x02,
		SendFailed = 0x03,
		NotAck     = 0x04,
		Argument   = 0x05,
	};

	enum Direction : uint8_t {
		Above = 0x00,
		Below = 0x01,
	};

	enum Unit : uint8_t {
		Torr     = 0x00,
		MilliBar = 0x01,
		Pascal   = 0x02,
	};
private:
	uint8_t id;
	Unit    unit;
public:
	MKS_902B() {}

	Result initialize(uint8_t id, Unit unit) {
		this->id = id;
		this->unit = unit;
		return setPressureUnit(Unit::Pascal);
	}

	Result enableSetpoint(int sp) {
		return setSetpointEnabled(sp, true);
	}

	Result disableSetpoint(int sp) {
		return setSetpointEnabled(sp, false);
	}

	Result configureSetpoint(int sp, float value, float hystOffset, Direction dir, bool enabled) {
		if (hystOffset < 0.0) {
			return Result::Argument;
		}

		float hyst = value;
		if (dir == Above) {
			hyst -= hystOffset;
		} else {
			hyst += hystOffset;
		}

		if (hyst < 0.0) {
			return Result::Argument;
		}

		value = fromUserUnit(value);
		hyst  = fromUserUnit(hyst);

		auto r = setSetpointValue(sp, value);
		if (r != Result::Success) { return r; }
		r = setSetpointDirection(sp, dir);
		if (r != Result::Success) { return r; }
		r = setSetpointHysteresis(sp, hyst);
		if (r != Result::Success) { return r; }
		return setSetpointEnabled(sp, enabled);
	}

	Result getSetpointStatus(int sp, bool & result) {
		if (sp < 1 || sp > 3) {
			return Result::Argument;
		}

		Serial.print('@');
		Serial.print(int(id));
		Serial.print("SS");
		Serial.print(sp);
		Serial.print("?");
		Serial.print(";FF");

		uint8_t from = 0;
		Result r = readBegin(&from);
		if (r != Result::Success) {
			return r;
		}

		int a = Serial.read();
		if (a < 0) { return Result::Failed; }
		int b = Serial.read();
		if (b < 0) { return Result::Failed; }
		int c = Serial.read();
		if (c < 0) { return Result::Failed; }

		if (a == 'S' && b == 'E' && c == 'T') {
			result = true;
		} else if (a == 'C' && b == 'L' && c == 'E') {
			result = false;
			Serial.read(); // a
			Serial.read(); // r
		} else {
			return Result::Failed;
		}

		return readEnd();
	}

	Result getPressure(float & result) {
		Serial.print('@');
		Serial.print(int(id));
		Serial.print("PR1?;FF");

		uint8_t from = 0;
		Result r = readBegin(&from);
		if (r != Result::Success) {
			return r;
		}

		result = Serial.parseFloat();
		// Round to two decimal places
		result = toUserUnit(result);

		return readEnd();
	}
private:
	float toUserUnit(float own) {
		if (unit == Unit::Torr) {
			return own * 0.0075006168270417;
		} else if (unit == Unit::MilliBar) {
			return own * 0.01;
		}
		return own;
	}

	float fromUserUnit(float user) {
		if (unit == Unit::Torr) {
			return user * 133.3223684210526;
		} else if (unit == Unit::MilliBar) {
			return user * 100.0;
		}
		return user;
	}
	// Setpoint value
	Result setSetpointValue(int sp, float value) {
		if (value < 0.0 || sp < 1 || sp > 3) {
			return Result::Failed;
		}

		Serial.print('@');
		Serial.print(int(id));
		Serial.print("SP");
		Serial.print(sp);
		Serial.print("!");
		Serial.print(value, 1);
		Serial.print(";FF");

		uint8_t from = 0;
		Result r = readBegin(&from);
		if (r != Result::Success) {
			return r;
		}
		Serial.parseFloat();
		return readEnd();
	}

	// Hysteresis value, if direction is Above, then this is
	// the lower point, if direction is below, then this is upper point
	// This value is absolute, not relative!!!
	// Gets automatically set to 10% of setpoint
	Result setSetpointHysteresis(int sp, float value) {
		if (value < 0.0 || sp < 1 || sp > 3) {
			return Result::Failed;
		}

		Serial.print('@');
		Serial.print(int(id));
		Serial.print("SH");
		Serial.print(sp);
		Serial.print("!");
		Serial.print(value, 1);
		Serial.print(";FF");

		uint8_t from = 0;
		Result r = readBegin(&from);
		if (r != Result::Success) {
			return r;
		}
		Serial.parseFloat();
		return readEnd();
	}

	// Set direction. Could be MKS_902B::Above or MKS_902B::Below
	Result setSetpointDirection(int sp, Direction dir) {
		if (sp < 1 || sp > 3) {
			return Result::Failed;
		}

		Serial.print('@');
		Serial.print(int(id));
		Serial.print("SD");
		Serial.print(sp);
		Serial.print("!");
		if (dir == Above) {
			Serial.print("ABOVE");
		} else {
			Serial.print("BELOW");
		}
		Serial.print(";FF");

		uint8_t from = 0;
		Result r = readBegin(&from);
		if (r != Result::Success) {
			return r;
		}
		Serial.read(); // a b
		Serial.read(); // b e
		Serial.read(); // o l
		Serial.read(); // v o
		Serial.read(); // e w
		return readEnd();
	}

	Result setSetpointEnabled(int sp, bool enabled) {
		if (sp < 1 || sp > 3) {
			return Result::Failed;
		}

		Serial.print('@');
		Serial.print(int(id));
		Serial.print("EN");
		Serial.print(sp);
		Serial.print("!");
		if (enabled) {
			Serial.print("ON");
		} else {
			Serial.print("OFF");
		}
		Serial.print(";FF");

		uint8_t from = 0;
		Result r = readBegin(&from);
		if (r != Result::Success) {
			return r;
		}
		Serial.read(); // o o
		Serial.read(); // n f
		if (!enabled) {
			Serial.read(); // f
		}
		return readEnd();
	}
private:
	// Read command header "@addrACK/NAK;FF"
	// if NAK is returned, calls readEnd and then returns NotAck
	Result readBegin(uint8_t * addr) {
		if (addr == nullptr) {
			return Result::Failed;
		}

		if (Serial.read() != '@') {
			return Result::Failed;
		}

		int id = Serial.parseInt();
		if (id < 1 || id > 255) {
			return Result::Failed;
		}

		*addr = uint8_t(id & 0xFF);

		int a = Serial.read();
		if (a < 0) { return Result::Failed; }
		int b = Serial.read();
		if (b < 0) { return Result::Failed; }
		int c = Serial.read();
		if (c < 0) { return Result::Failed; }

		int error = -1;
		if (a == 'A' && b == 'C' && c == 'K') {
			// ACK - success
		} else if (a == 'N' && b == 'A' && c == 'K') {
			// NAK - fail
			error = Serial.parseInt();
		} else {
			// Only ACK and NAK expected
			return Result::NotAck;
		}

		if (error != -1) {
			// Parse out ;FF
			readEnd();
			return Result::Failed;
		}

		return Result::Success;
	}

	// Tries to read command end, aka ";FF" characters
	// If it fails even on a single one, then immediatly returns Failed
	// Else if everything wen't fine, returns Success.
	Result readEnd() {
		if (Serial.read() != ';' ||
			Serial.read() != 'F' ||
			Serial.read() != 'F')
		{
			return Result::Failed;
		}
		return Result::Success;
	}

	// Sets pressure unit to be returned.
	Result setPressureUnit(Unit unit) {
		Serial.print('@');
		Serial.print(int(id));
		Serial.print("U!");
		if (unit == Torr) {
			Serial.print("TORR");
		} else if (unit == MilliBar) {
			Serial.print("MBAR");
		} else {
			Serial.print("PASCAL");
		}
		Serial.print(";FF");

		uint8_t from = 0;
		Result r = readBegin(&from);
		if (r != Result::Success) {
			return r;
		}
		Serial.read(); // t m p
		Serial.read(); // o b a
		Serial.read(); // r a s
		Serial.read(); // r r c
		if (unit != Torr && unit != MilliBar) {
			Serial.read(); // a
			Serial.read(); // l
		}
		return readEnd();
	}
};
