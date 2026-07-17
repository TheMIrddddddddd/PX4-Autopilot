############################################################################
#
#   Copyright (c) 2026 PX4 Development Team. All rights reserved.
#
############################################################################

px4_add_module(
	MODULE drivers__imu__jy901b__tests
	MAIN jy901b_parser_test
	SRCS
		JY901B_ParserTest.cpp
		../JY901B_Parser.cpp
	)
