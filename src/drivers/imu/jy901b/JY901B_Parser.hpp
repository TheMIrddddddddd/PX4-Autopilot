/****************************************************************************
 *
 *   Copyright (c) 2026 PX4 Development Team. All rights reserved.
 *
 ****************************************************************************/

#pragma once

#include <cstdint>

class JY901BParser
{
public:
	static constexpr uint8_t FRAME_HEADER{0x55};
	static constexpr uint8_t FRAME_SIZE{11};
	static constexpr uint8_t PAYLOAD_SIZE{8};

	struct Frame {
		uint8_t type{0};
		uint8_t payload[PAYLOAD_SIZE] {};
	};

	enum class Result : uint8_t {
		None = 0,
		FrameComplete,
		ChecksumError,
		Discarded
	};

	Result parse(uint8_t byte, Frame &frame);
	void reset() { _index = 0; _discarded_bytes = 0; }
	uint8_t discarded_bytes() const { return _discarded_bytes; }

private:
	uint8_t resynchronize();

	uint8_t _buffer[FRAME_SIZE] {};
	uint8_t _index{0};
	uint8_t _discarded_bytes{0};
};
