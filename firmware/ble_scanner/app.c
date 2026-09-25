#include "app.h"

#include <inttypes.h>

#include "app_assert.h"
#include "scan_tracker.h"
#include "sl_bluetooth.h"
#include "sl_iostream.h"
#include "sl_power_manager.h"
#include "sl_sleeptimer.h"

#define SCAN_INTERVAL_UNITS 160U
#define SCAN_WINDOW_UNITS 80U

static scan_tracker_t scan_tracker;

static uint32_t uptime_ms(void) {
  return sl_sleeptimer_tick_to_ms(sl_sleeptimer_get_tick_count());
}

static const char *log_reason_name(scan_log_reason_t reason) {
  switch (reason) {
  case SCAN_LOG_FIRST_SEEN:
    return "first";
  case SCAN_LOG_NAME_CHANGED:
    return "name";
  case SCAN_LOG_RSSI_CHANGED:
    return "rssi";
  case SCAN_LOG_PERIODIC_REFRESH:
    return "refresh";
  case SCAN_LOG_NONE:
  default:
    return "none";
  }
}

static void log_observation(const scan_observation_result_t *result,
                            uint32_t now_ms) {
  const scan_device_snapshot_t *device = &result->device;
  const char *name = (device->name[0] != '\0') ? device->name : "<unknown>";

  // RTT is configured in non-blocking skip mode. A full host buffer may drop a
  // line, but it must never stall the Bluetooth event handler.
  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "scan t=%" PRIu32 " addr=%02X:%02X:%02X:%02X:%02X:%02X type=%u rssi=%d"
      " name=\"%s\" reason=%s reports=%" PRIu32 "%s\r\n",
      now_ms, device->address[5], device->address[4], device->address[3],
      device->address[2], device->address[1], device->address[0],
      device->address_type, device->rssi, name, log_reason_name(result->reason),
      device->report_count, result->malformed_payload ? " malformed-ad" : "");
}

static void log_summary(uint32_t now_ms) {
  const scan_tracker_stats_t stats =
      scan_tracker_snapshot_summary(&scan_tracker, now_ms);

  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "summary t=%" PRIu32 " reports=%" PRIu32 " discoveries=%" PRIu32
      " active=%" PRIu32 " evictions=%" PRIu32 " suppressed=%" PRIu32
      " malformed=%" PRIu32 "\r\n",
      now_ms, stats.total_reports, stats.discoveries, stats.active_devices,
      stats.cache_evictions, stats.suppressed_logs, stats.malformed_payloads);
}

void app_init(void) {
  scan_tracker_init(&scan_tracker, uptime_ms());

  // RTT lives in target RAM and is not readable by the debug probe in EM2.
  // Keep EM1 as the lowest sleep mode for the lifetime of this logging build.
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "BLE scanner initialized; waiting for stack boot\r\n");
}

void app_process_action(void) {
  const uint32_t now_ms = uptime_ms();
  if (scan_tracker_summary_due(&scan_tracker, now_ms)) {
    log_summary(now_ms);
  }
}

void sl_bt_on_event(sl_bt_msg_t *event) {
  sl_status_t status;

  switch (SL_BT_MSG_ID(event->header)) {
  case sl_bt_evt_system_boot_id:
    status =
        sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive,
                                     SCAN_INTERVAL_UNITS, SCAN_WINDOW_UNITS);
    app_assert_status(status);

    status = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                                 sl_bt_scanner_discover_observation);
    app_assert_status(status);

    (void)sl_iostream_printf(
        SL_IOSTREAM_STDOUT,
        "BLE scan started: passive, interval=100ms, window=50ms, PHY=1M\r\n");
    break;

  case sl_bt_evt_scanner_legacy_advertisement_report_id: {
    const sl_bt_evt_scanner_legacy_advertisement_report_t *report =
        &event->data.evt_scanner_legacy_advertisement_report;
    scan_observation_t observation = {
        .address_type = report->address_type,
        .rssi = report->rssi,
        .payload = report->data.data,
        .payload_length = report->data.len,
    };

    for (size_t index = 0U; index < SCAN_TRACKER_ADDRESS_SIZE; ++index) {
      observation.address[index] = report->address.addr[index];
    }

    const uint32_t now_ms = uptime_ms();
    const scan_observation_result_t result =
        scan_tracker_observe(&scan_tracker, &observation, now_ms);
    if (result.reason != SCAN_LOG_NONE) {
      log_observation(&result, now_ms);
    }
    break;
  }

  default:
    break;
  }
}
