/****************************************************************************
 *
 *   Copyright (c) 2026 PX4 Development Team. All rights reserved.
 *
 ****************************************************************************/

#include "JY901B.hpp"

#include <cerrno>
#include <cmath>
#include <cstring>
#include <inttypes.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <lib/drivers/device/Device.hpp>
#include <px4_platform_common/log.h>
#include <px4_platform_common/posix.h>

namespace
{
static constexpr uint8_t UNLOCK_COMMAND[5] {0xFF, 0xAA, 0x69, 0x88, 0xB5};
static constexpr uint8_t OUTPUT_COMMAND[5] {0xFF, 0xAA, 0x02, 0x56, 0x00};
static constexpr uint8_t RATE_COMMAND[5] {0xFF, 0xAA, 0x03, 0x0B, 0x00};
}

JY901B::JY901B(const char *port, enum Rotation rotation) :
	ScheduledWorkItem(MODULE_NAME, px4::serial_port_to_wq(port)),
	_px4_accel(make_device_id(port, DRV_IMU_DEVTYPE_JY901B), rotation),
	_px4_gyro(make_device_id(port, DRV_IMU_DEVTYPE_JY901B), rotation),
	_px4_mag(make_device_id(port, DRV_MAG_DEVTYPE_JY901B), rotation)
{
	strncpy(_port, port, sizeof(_port) - 1);
	_port[sizeof(_port) - 1] = '\0';

	_px4_accel.set_range(16.f * CONSTANTS_ONE_G);
	_px4_accel.set_scale(16.f * CONSTANTS_ONE_G / 32768.f);
	_px4_gyro.set_range(math::radians(2000.f));
	_px4_gyro.set_scale(math::radians(2000.f / 32768.f));
	_px4_mag.set_scale(1.f / 12000.f); // 12000 LSB/Gauss, verified against the on-board field norm.
}

JY901B::~JY901B()
{
	request_stop_and_wait();
	perf_free(_cycle_perf);
	perf_free(_read_errors);
	perf_free(_checksum_errors);
	perf_free(_discarded_bytes);
	perf_free(_timeouts);
}

uint32_t JY901B::make_device_id(const char *port, uint8_t devtype)
{
	device::Device::DeviceId id{};
	id.devid_s.bus_type = device::Device::DeviceBusType_SERIAL;
	id.devid_s.devtype = devtype;

	const size_t length = strlen(port);

	if (length > 0 && port[length - 1] >= '0' && port[length - 1] <= '9') {
		id.devid_s.bus = static_cast<uint8_t>(port[length - 1] - '0');
	}

	return id.devid;
}

int16_t JY901B::int16_le(const uint8_t *data)
{
	return static_cast<int16_t>(static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8));
}

int32_t JY901B::int32_le(const uint8_t *data)
{
	return static_cast<int32_t>(static_cast<uint32_t>(data[0]) |
				    (static_cast<uint32_t>(data[1]) << 8) |
				    (static_cast<uint32_t>(data[2]) << 16) |
				    (static_cast<uint32_t>(data[3]) << 24));
}

int JY901B::open_serial_port()
{
	if (_fd >= 0) {
		return PX4_OK;
	}

	_fd = ::open(_port, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (_fd < 0) {
		PX4_ERR("open %s failed (%i)", _port, errno);
		return PX4_ERROR;
	}

	termios uart_config{};

	if (tcgetattr(_fd, &uart_config) != 0) {
		PX4_ERR("tcgetattr failed (%i)", errno);
		close_serial_port();
		return PX4_ERROR;
	}

	cfmakeraw(&uart_config);
	uart_config.c_cflag |= (CLOCAL | CREAD);
	uart_config.c_cflag &= ~(CSTOPB | PARENB | CRTSCTS);
	uart_config.c_cflag &= ~CSIZE;
	uart_config.c_cflag |= CS8;
	uart_config.c_cc[VMIN] = 0;
	uart_config.c_cc[VTIME] = 0;

	if (cfsetispeed(&uart_config, B115200) != 0 ||
	    cfsetospeed(&uart_config, B115200) != 0 ||
	    tcsetattr(_fd, TCSANOW, &uart_config) != 0) {
		PX4_ERR("configure %s failed (%i)", _port, errno);
		close_serial_port();
		return PX4_ERROR;
	}

	tcflush(_fd, TCIOFLUSH);
	return PX4_OK;
}

bool JY901B::write_command(const uint8_t command[5])
{
	size_t offset = 0;
	const hrt_abstime deadline = hrt_absolute_time() + WRITE_TIMEOUT_US;

	while (offset < 5 && hrt_absolute_time() < deadline) {
		const ssize_t written = ::write(_fd, &command[offset], 5 - offset);

		if (written > 0) {
			offset += static_cast<size_t>(written);
			continue;
		}

		if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
			PX4_ERR("configuration write failed (%i)", errno);
			return false;
		}

		px4_usleep(1_ms);
	}

	if (offset != 5) {
		PX4_ERR("configuration write timeout (%u/5)", static_cast<unsigned>(offset));
		return false;
	}

	return true;
}

bool JY901B::configure_sensor()
{
	if (!write_command(UNLOCK_COMMAND)) {
		return false;
	}

	px4_usleep(200_ms);

	if (!write_command(OUTPUT_COMMAND)) {
		return false;
	}

	px4_usleep(100_ms);

	if (!write_command(RATE_COMMAND)) {
		return false;
	}

	px4_usleep(100_ms);
	tcflush(_fd, TCIFLUSH);
	_parser.reset();
	return true;
}

void JY901B::close_serial_port()
{
	if (_fd >= 0) {
		::close(_fd);
		_fd = -1;
	}
}

int JY901B::init()
{
	ScheduleNow();
	const hrt_abstime timeout = hrt_absolute_time() + INIT_TIMEOUT_US;

	while (!_task_initialized.load() && !_task_init_failed.load() && hrt_absolute_time() < timeout) {
		px4_usleep(10_ms);
	}

	if (_task_initialized.load()) {
		return PX4_OK;
	}

	PX4_ERR("work queue initialization failed");
	request_stop_and_wait();
	return PX4_ERROR;
}

void JY901B::request_stop_and_wait()
{
	if (_task_exited.load()) {
		return;
	}

	_task_should_exit.store(true);
	ScheduleNow();
	const hrt_abstime timeout = hrt_absolute_time() + INIT_TIMEOUT_US;

	while (!_task_exited.load() && hrt_absolute_time() < timeout) {
		px4_usleep(10_ms);
	}

	if (!_task_exited.load()) {
		PX4_ERR("work queue did not stop");
	}
}

void JY901B::process_frame(const JY901BParser::Frame &frame, hrt_abstime timestamp_sample)
{
	// JY901B uses x forward, y left, z up. Normalize to the PX4 FRD frame before applying -R.
	const float x = int16_le(&frame.payload[0]);
	const float y = -static_cast<float>(int16_le(&frame.payload[2]));
	const float z = -static_cast<float>(int16_le(&frame.payload[4]));

	switch (frame.type) {
	case TYPE_ACCEL: {
		const float temperature = int16_le(&frame.payload[6]) / 100.f;
		_px4_accel.set_temperature(temperature);
		_px4_accel.set_error_count(perf_event_count(_read_errors) + perf_event_count(_checksum_errors));
		_px4_accel.update(timestamp_sample, x, y, z);
		_frame_count_accel++;
		break;
	}

	case TYPE_GYRO: {
		const float temperature = int16_le(&frame.payload[6]) / 100.f;
		_px4_gyro.set_temperature(temperature);
		_px4_gyro.set_error_count(perf_event_count(_read_errors) + perf_event_count(_checksum_errors));
		_px4_gyro.update(timestamp_sample, x, y, z);
		_frame_count_gyro++;
		break;
	}

	case TYPE_MAG:
		_px4_mag.set_error_count(perf_event_count(_read_errors) + perf_event_count(_checksum_errors));
		_px4_mag.update(timestamp_sample, x, y, z);
		_frame_count_mag++;
		break;

	case TYPE_PRESSURE: {
		sensor_baro_s report{};
		report.timestamp_sample = timestamp_sample;
		report.device_id = make_device_id(_port, DRV_BARO_DEVTYPE_JY901B);
		report.pressure = static_cast<float>(int32_le(&frame.payload[0]));
		report.temperature = NAN;
		report.error_count = perf_event_count(_read_errors) + perf_event_count(_checksum_errors);
		report.timestamp = hrt_absolute_time();
		_sensor_baro_pub.publish(report);
		_last_height_cm = int32_le(&frame.payload[4]);
		_frame_count_pressure++;
		break;
	}

	default:
		_frame_count_unknown++;
		break;
	}
}

void JY901B::Run()
{
	if (_task_should_exit.load()) {
		close_serial_port();
		ScheduleClear();
		_task_exited.store(true);
		return;
	}

	if (!_task_initialized.load()) {
		if (open_serial_port() != PX4_OK || !configure_sensor()) {
			close_serial_port();
			_task_init_failed.store(true);
			ScheduleClear();
			_task_exited.store(true);
			return;
		}

		_start_timestamp = hrt_absolute_time();
		_last_run_timestamp = _start_timestamp;
		_task_initialized.store(true);
		ScheduleOnInterval(RUN_INTERVAL_US, RUN_INTERVAL_US);
		return;
	}

	perf_begin(_cycle_perf);
	const hrt_abstime now = hrt_absolute_time();
	const uint32_t run_interval = static_cast<uint32_t>(now - _last_run_timestamp);
	_last_run_timestamp = now;

	if (run_interval > _max_run_interval_us) {
		_max_run_interval_us = run_interval;
	}

	uint8_t buffer[256];
	ssize_t bytes_read = 0;

	do {
		bytes_read = ::read(_fd, buffer, sizeof(buffer));

		if (bytes_read > 0) {
			_bytes_received += bytes_read;

			for (ssize_t i = 0; i < bytes_read; i++) {
				JY901BParser::Frame frame{};
				const JY901BParser::Result result = _parser.parse(buffer[i], frame);

				if (result == JY901BParser::Result::FrameComplete) {
					const hrt_abstime timestamp_sample = hrt_absolute_time();
					_last_frame_timestamp = timestamp_sample;
					_timeout_reported = false;
					process_frame(frame, timestamp_sample);

				} else if (result == JY901BParser::Result::ChecksumError) {
					perf_count(_checksum_errors);

					for (uint8_t discarded = 0; discarded < _parser.discarded_bytes(); discarded++) {
						perf_count(_discarded_bytes);
					}

				} else if (result == JY901BParser::Result::Discarded) {
					perf_count(_discarded_bytes);
				}
			}

		} else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
			perf_count(_read_errors);
			break;
		}
	} while (bytes_read > 0);

	const hrt_abstime timeout_reference = (_last_frame_timestamp != 0) ? _last_frame_timestamp : _start_timestamp;

	if (timeout_reference != 0 && hrt_elapsed_time(&timeout_reference) > DATA_TIMEOUT_US && !_timeout_reported) {
		perf_count(_timeouts);
		_timeout_reported = true;
	}

	perf_end(_cycle_perf);
}

void JY901B::print_status()
{
	perf_print_counter(_cycle_perf);
	perf_print_counter(_read_errors);
	perf_print_counter(_checksum_errors);
	perf_print_counter(_discarded_bytes);
	perf_print_counter(_timeouts);

	const float elapsed_s = (_start_timestamp != 0) ? (hrt_elapsed_time(&_start_timestamp) * 1e-6f) : 0.f;
	const float divisor = (elapsed_s > 0.f) ? elapsed_s : 1.f;

	PX4_INFO("port: %s, bytes: %llu", _port, static_cast<unsigned long long>(_bytes_received));
	PX4_INFO("rates: accel %.1f, gyro %.1f, mag %.1f, baro %.1f Hz",
		 (double)(_frame_count_accel / divisor), (double)(_frame_count_gyro / divisor),
		 (double)(_frame_count_mag / divisor), (double)(_frame_count_pressure / divisor));
	PX4_INFO("unknown frames: %" PRIu32 ", last height: %.2f m", _frame_count_unknown,
		 (double)(_last_height_cm * 0.01f));
	PX4_INFO("last frame age: %llu us, max run interval: %" PRIu32 " us",
		 static_cast<unsigned long long>(_last_frame_timestamp ? hrt_elapsed_time(&_last_frame_timestamp) : 0),
		 _max_run_interval_us);
}
