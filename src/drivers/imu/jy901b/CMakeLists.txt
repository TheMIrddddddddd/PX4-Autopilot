############################################################################
#
#   Copyright (c) 2026 PX4 Development Team. All rights reserved.
#
############################################################################

px4_add_module(
	MODULE drivers__imu__jy901b
	MAIN jy901b
	SRCS
		JY901B.cpp
		JY901B.hpp
		JY901B_Parser.cpp
		JY901B_Parser.hpp
		jy901b_main.cpp
	DEPENDS
		drivers_accelerometer
		drivers_gyroscope
		drivers_magnetometer
		px4_work_queue
	MODULE_CONFIG
		module.yaml
	)

if(PX4_TESTING)
	add_subdirectory(tests)
endif()
