/****************************************************************************
 *
 *   Copyright (c) 2026 PX4 Development Team. All rights reserved.
 *
 ****************************************************************************/

#pragma once

#include "JY901B_Parser.hpp"

#include <drivers/drv_hrt.h>
#include <drivers/drv_sensor.h>
#include <lib/conversion/rotation.h>
#include <lib/drivers/accelerometer/PX4Accelerometer.hpp>
#include <lib/drivers/gyroscope/PX4Gyroscope.hpp>
#include <lib/drivers/magnetometer/PX4Magnetometer.hpp>
#include <perf/perf_counter.h>
#include <px4_platform_common/atomic.h>
#include <px4_platform_common/px4_work_queue/ScheduledWorkItem.hpp>
#include <uORB/PublicationMulti.hpp>
#include <uORB/topics/sensor_baro.h>

using namespace time_literals;

class JY901B : public px4::ScheduledWorkItem
{
public:
	JY901B(const char *port, enum Rotation rotation);
	~JY901B() override;

	int init();
	void request_stop_and_wait();
	void print_status();

private:
	static constexpr uint32_t RUN_INTERVAL_US{2000};
	static constexpr uint32_t DATA_TIMEOUT_US{100000};
	static constexpr uint32_t INIT_TIMEOUT_US{2000000};
	static constexpr uint32_t WRITE_TIMEOUT_US{20000};

	static constexpr uint8_t TYPE_ACCEL{0x51};
	static constexpr uint8_t TYPE_GYRO{0x52};
	static constexpr uint8_t TYPE_MAG{0x54};
	static constexpr uint8_t TYPE_PRESSURE{0x56};

	static uint32_t make_device_id(const char *port, uint8_t devtype);
	static int16_t int16_le(const uint8_t *data);
	static int32_t int32_le(const uint8_t *data);

	int open_serial_port();
	bool configure_sensor();
	bool write_command(const uint8_t command[5]);
	void close_serial_port();
	void process_frame(const JY901BParser::Frame &frame, hrt_abstime timestamp_sample);
	void Run() override;

	char _port[20] {};
	int _fd{-1};

	JY901BParser _parser;
	PX4Accelerometer _px4_accel;
	PX4Gyroscope _px4_gyro;
	PX4Magnetometer _px4_mag;
	uORB::PublicationMulti<sensor_baro_s> _sensor_baro_pub{ORB_ID(sensor_baro)};

	perf_counter_t _cycle_perf{perf_alloc(PC_ELAPSED, MODULE_NAME": cycle")};
	perf_counter_t _read_errors{perf_alloc(PC_COUNT, MODULE_NAME": read error")};
	perf_counter_t _checksum_errors{perf_alloc(PC_COUNT, MODULE_NAME": checksum error")};
	perf_counter_t _discarded_bytes{perf_alloc(PC_COUNT, MODULE_NAME": discarded byte")};
	perf_counter_t _timeouts{perf_alloc(PC_COUNT, MODULE_NAME": timeout")};

	uint64_t _bytes_received{0};
	uint32_t _frame_count_accel{0};
	uint32_t _frame_count_gyro{0};
	uint32_t _frame_count_mag{0};
	uint32_t _frame_count_pressure{0};
	uint32_t _frame_count_unknown{0};
	int32_t _last_height_cm{0};
	hrt_abstime _start_timestamp{0};
	hrt_abstime _last_frame_timestamp{0};
	hrt_abstime _last_run_timestamp{0};
	uint32_t _max_run_interval_us{0};
	bool _timeout_reported{false};
	px4::atomic_bool _task_should_exit{false};
	px4::atomic_bool _task_exited{false};
	px4::atomic_bool _task_initialized{false};
	px4::atomic_bool _task_init_failed{false};
};
