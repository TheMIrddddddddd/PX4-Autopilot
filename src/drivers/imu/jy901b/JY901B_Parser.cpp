/****************************************************************************
 *
 *   Copyright (c) 2026 PX4 Development Team. All rights reserved.
 *
 ****************************************************************************/

#include "JY901B_Parser.hpp"

#include <cstring>

JY901BParser::Result JY901BParser::parse(uint8_t byte, Frame &frame)
{
	_discarded_bytes = 0;

	if (_index == 0) {
		if (byte != FRAME_HEADER) {
			_discarded_bytes = 1;
			return Result::Discarded;
		}

		_buffer[_index++] = byte;
		return Result::None;
	}

	_buffer[_index++] = byte;

	if (_index < FRAME_SIZE) {
		return Result::None;
	}

	uint8_t checksum = 0;

	for (uint8_t i = 0; i < FRAME_SIZE - 1; i++) {
		checksum = static_cast<uint8_t>(checksum + _buffer[i]);
	}

	if (checksum == _buffer[FRAME_SIZE - 1]) {
		frame.type = _buffer[1];
		memcpy(frame.payload, &_buffer[2], PAYLOAD_SIZE);
		_index = 0;
		return Result::FrameComplete;
	}

	_discarded_bytes = resynchronize();
	return Result::ChecksumError;
}

uint8_t JY901BParser::resynchronize()
{
	for (uint8_t i = 1; i < FRAME_SIZE; i++) {
		if (_buffer[i] == FRAME_HEADER) {
			const uint8_t remaining = FRAME_SIZE - i;
			memmove(_buffer, &_buffer[i], remaining);
			_index = remaining;
			return i;
		}
	}

	_index = 0;
	return FRAME_SIZE;
}
