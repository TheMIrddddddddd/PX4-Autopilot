/****************************************************************************
 *
 *   Copyright (c) 2026 PX4 Development Team. All rights reserved.
 *
 ****************************************************************************/

#include "JY901B.hpp"

#include <cstring>

#include <px4_platform_common/cli.h>
#include <px4_platform_common/getopt.h>
#include <px4_platform_common/log.h>
#include <px4_platform_common/module.h>

namespace jy901b
{
JY901B *g_dev{nullptr};

int start(const char *port, enum Rotation rotation)
{
	if (g_dev != nullptr) {
		PX4_WARN("already started");
		return PX4_OK;
	}

	if (port == nullptr) {
		PX4_ERR("serial device is required");
		return PX4_ERROR;
	}

	g_dev = new JY901B(port, rotation);

	if (g_dev == nullptr || g_dev->init() != PX4_OK) {
		PX4_ERR("driver start failed");
		delete g_dev;
		g_dev = nullptr;
		return PX4_ERROR;
	}

	return PX4_OK;
}

int stop()
{
	if (g_dev == nullptr) {
		PX4_WARN("not running");
		return PX4_ERROR;
	}

	delete g_dev;
	g_dev = nullptr;
	return PX4_OK;
}

int status()
{
	if (g_dev == nullptr) {
		PX4_WARN("not running");
		return PX4_ERROR;
	}

	g_dev->print_status();
	return PX4_OK;
}

int usage()
{
	PRINT_MODULE_DESCRIPTION(
		R"DESCR_STR(
### Description

Serial driver for the WitMotion JY901B IMU, magnetometer and barometer.
The sensor must be connected to a dedicated UART and configured for 115200 baud.

### Examples

$ jy901b start -d /dev/ttyS3 -R 0
$ jy901b status
$ jy901b stop
)DESCR_STR");
	PRINT_MODULE_USAGE_NAME("jy901b", "driver");
	PRINT_MODULE_USAGE_SUBCATEGORY("imu");
	PRINT_MODULE_USAGE_COMMAND_DESCR("start", "Start driver");
	PRINT_MODULE_USAGE_PARAM_STRING('d', "/dev/ttyS3", nullptr, "Serial device", true);
	PRINT_MODULE_USAGE_PARAM_INT('R', 0, 0, 40, "Sensor rotation", true);
	PRINT_MODULE_USAGE_COMMAND_DESCR("status", "Print driver status");
	PRINT_MODULE_USAGE_COMMAND_DESCR("stop", "Stop driver");
	return PX4_OK;
}
}

extern "C" __EXPORT int jy901b_main(int argc, char *argv[])
{
	const char *device_path = "/dev/ttyS3";
	enum Rotation rotation = ROTATION_NONE;
	int myoptind = 1;
	const char *myoptarg = nullptr;
	int ch;

	while ((ch = px4_getopt(argc, argv, "d:R:", &myoptind, &myoptarg)) != EOF) {
		switch (ch) {
		case 'd':
			device_path = myoptarg;
			break;

		case 'R': {
			int value = 0;

			if (px4_get_parameter_value(myoptarg, value) != 0 || value < 0 || value > 40) {
				PX4_ERR("invalid rotation");
				return PX4_ERROR;
			}

			rotation = static_cast<enum Rotation>(value);
			break;
		}

		default:
			return jy901b::usage();
		}
	}

	if (myoptind >= argc) {
		return jy901b::usage();
	}

	if (!strcmp(argv[myoptind], "start")) {
		return jy901b::start(device_path, rotation);

	} else if (!strcmp(argv[myoptind], "status")) {
		return jy901b::status();

	} else if (!strcmp(argv[myoptind], "stop")) {
		return jy901b::stop();
	}

	return jy901b::usage();
}
