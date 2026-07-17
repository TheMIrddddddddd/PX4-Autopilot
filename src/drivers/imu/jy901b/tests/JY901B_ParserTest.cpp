/****************************************************************************
 *
 *   Copyright (c) 2026 PX4 Development Team. All rights reserved.
 *
 ****************************************************************************/

#include <unit_test.h>

#include "../JY901B_Parser.hpp"

#include <cstring>

extern "C" __EXPORT int jy901b_parser_test_main(int argc, char *argv[]);

class JY901BParserTest : public UnitTest
{
public:
	bool run_tests() override;

private:
	bool parseValidFrames();
	bool parseSplitAndConsecutiveFrames();
	bool recoverFromNoiseAndBadChecksum();
	bool recoverAfterDroppedByte();
	bool decodeSignedPayload();

	static void finalize(uint8_t frame[JY901BParser::FRAME_SIZE]);
};

void JY901BParserTest::finalize(uint8_t frame[JY901BParser::FRAME_SIZE])
{
	uint8_t checksum = 0;

	for (uint8_t i = 0; i < JY901BParser::FRAME_SIZE - 1; i++) {
		checksum = static_cast<uint8_t>(checksum + frame[i]);
	}

	frame[JY901BParser::FRAME_SIZE - 1] = checksum;
}

bool JY901BParserTest::parseValidFrames()
{
	const uint8_t types[] {0x51, 0x52, 0x54, 0x56};

	for (uint8_t type : types) {
		JY901BParser parser;
		JY901BParser::Frame parsed{};
		uint8_t frame[JY901BParser::FRAME_SIZE] {0x55, type, 1, 2, 3, 4, 5, 6, 7, 8, 0};
		finalize(frame);
		JY901BParser::Result result = JY901BParser::Result::None;

		for (uint8_t byte : frame) {
			result = parser.parse(byte, parsed);
		}

		ut_assert_true(result == JY901BParser::Result::FrameComplete);
		ut_assert_true(parsed.type == type);
		ut_assert_true(memcmp(parsed.payload, &frame[2], JY901BParser::PAYLOAD_SIZE) == 0);
	}

	return true;
}

bool JY901BParserTest::parseSplitAndConsecutiveFrames()
{
	JY901BParser parser;
	JY901BParser::Frame parsed{};
	uint8_t frame1[JY901BParser::FRAME_SIZE] {0x55, 0x51, 1, 2, 3, 4, 5, 6, 7, 8, 0};
	uint8_t frame2[JY901BParser::FRAME_SIZE] {0x55, 0x52, 8, 7, 6, 5, 4, 3, 2, 1, 0};
	finalize(frame1);
	finalize(frame2);

	for (uint8_t i = 0; i < 5; i++) {
		ut_assert_true(parser.parse(frame1[i], parsed) == JY901BParser::Result::None);
	}

	for (uint8_t i = 5; i < JY901BParser::FRAME_SIZE; i++) {
		const auto result = parser.parse(frame1[i], parsed);

		if (i == JY901BParser::FRAME_SIZE - 1) {
			ut_assert_true(result == JY901BParser::Result::FrameComplete);
		}
	}

	for (uint8_t byte : frame2) {
		parser.parse(byte, parsed);
	}

	ut_assert_true(parsed.type == 0x52);
	return true;
}

bool JY901BParserTest::recoverFromNoiseAndBadChecksum()
{
	JY901BParser parser;
	JY901BParser::Frame parsed{};
	ut_assert_true(parser.parse(0x12, parsed) == JY901BParser::Result::Discarded);
	ut_assert_true(parser.parse(0x34, parsed) == JY901BParser::Result::Discarded);

	uint8_t bad[JY901BParser::FRAME_SIZE] {0x55, 0x51, 1, 2, 3, 4, 5, 6, 7, 8, 0};
	finalize(bad);
	bad[10]++;
	JY901BParser::Result result = JY901BParser::Result::None;

	for (uint8_t byte : bad) {
		result = parser.parse(byte, parsed);
	}

	ut_assert_true(result == JY901BParser::Result::ChecksumError);
	ut_assert_true(parser.discarded_bytes() == JY901BParser::FRAME_SIZE);

	uint8_t good[JY901BParser::FRAME_SIZE] {0x55, 0x56, 0x23, 0x88, 0x01, 0x00, 0x39, 0x1f, 0x00, 0x00, 0};
	finalize(good);

	for (uint8_t byte : good) {
		result = parser.parse(byte, parsed);
	}

	ut_assert_true(result == JY901BParser::Result::FrameComplete);
	ut_assert_true(parsed.type == 0x56);
	return true;
}

bool JY901BParserTest::recoverAfterDroppedByte()
{
	JY901BParser parser;
	JY901BParser::Frame parsed{};
	uint8_t damaged[JY901BParser::FRAME_SIZE] {0x55, 0x51, 1, 2, 3, 4, 5, 6, 7, 8, 0};
	uint8_t good[JY901BParser::FRAME_SIZE] {0x55, 0x54, 8, 7, 6, 5, 4, 3, 2, 1, 0};
	finalize(damaged);
	finalize(good);

	// Drop one byte from the first frame, then append a complete frame. The next
	// header is consumed as byte 10 of the damaged frame and must be retained.
	for (uint8_t i = 0; i < JY901BParser::FRAME_SIZE - 1; i++) {
		parser.parse(damaged[i], parsed);
	}

	ut_assert_true(parser.parse(good[0], parsed) == JY901BParser::Result::ChecksumError);
	ut_assert_true(parser.discarded_bytes() == JY901BParser::FRAME_SIZE - 1);
	JY901BParser::Result result = JY901BParser::Result::None;

	for (uint8_t i = 1; i < JY901BParser::FRAME_SIZE; i++) {
		result = parser.parse(good[i], parsed);
	}

	ut_assert_true(result == JY901BParser::Result::FrameComplete);
	ut_assert_true(parsed.type == 0x54);
	ut_assert_true(memcmp(parsed.payload, &good[2], JY901BParser::PAYLOAD_SIZE) == 0);
	return true;
}

bool JY901BParserTest::decodeSignedPayload()
{
	uint8_t pressure_frame[JY901BParser::FRAME_SIZE] {0x55, 0x56, 0x23, 0x88, 0x01, 0x00, 0x39, 0x1f, 0x00, 0x00, 0};
	finalize(pressure_frame);
	JY901BParser parser;
	JY901BParser::Frame parsed{};

	for (uint8_t byte : pressure_frame) {
		parser.parse(byte, parsed);
	}

	const int32_t pressure = static_cast<int32_t>(static_cast<uint32_t>(parsed.payload[0]) |
				 (static_cast<uint32_t>(parsed.payload[1]) << 8) |
				 (static_cast<uint32_t>(parsed.payload[2]) << 16) |
				 (static_cast<uint32_t>(parsed.payload[3]) << 24));
	ut_assert_true(pressure == 100387);

	const uint8_t negative[2] {0xB7, 0xFD};
	const int16_t value = static_cast<int16_t>(static_cast<uint16_t>(negative[0]) |
			      (static_cast<uint16_t>(negative[1]) << 8));
	ut_assert_true(value == -585);
	return true;
}

bool JY901BParserTest::run_tests()
{
	ut_run_test(parseValidFrames);
	ut_run_test(parseSplitAndConsecutiveFrames);
	ut_run_test(recoverFromNoiseAndBadChecksum);
	ut_run_test(recoverAfterDroppedByte);
	ut_run_test(decodeSignedPayload);
	return (_tests_failed == 0);
}

ut_declare_test_c(jy901b_parser_test_main, JY901BParserTest)
