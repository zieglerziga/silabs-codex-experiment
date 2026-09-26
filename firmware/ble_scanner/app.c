#include "app.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_assert.h"
#include "rtt_command.h"
#include "scan_tracker.h"
#include "sl_bluetooth.h"
#include "sl_iostream.h"
#include "sl_power_manager.h"
#include "sl_sleeptimer.h"

#define RTT_COMMAND_LINE_CAPACITY 96U
#define RTT_READ_CHUNK_SIZE 16U

typedef struct {
  uint8_t mode;
  uint16_t interval;
  uint16_t window;
  uint8_t phy;
  uint8_t discover;
  uint32_t flags;
  uint8_t filter;
} scan_config_t;

typedef struct {
  scan_config_t scan;
  bool scanning;
  bool stack_ready;
  bool log_observations;
  bool log_summaries;
} runtime_state_t;

static scan_tracker_t scan_tracker;
static runtime_state_t runtime_state = {
    .scan =
        {
            .mode = sl_bt_scanner_scan_mode_passive,
            .interval = 160U,
            .window = 80U,
            .phy = sl_bt_scanner_scan_phy_1m,
            .discover = sl_bt_scanner_discover_observation,
            .flags = 0U,
            .filter = 0U,
        },
    .scanning = false,
    .stack_ready = false,
    .log_observations = true,
    .log_summaries = true,
};
static char rtt_command_line[RTT_COMMAND_LINE_CAPACITY];
static size_t rtt_command_line_length;
static bool rtt_command_line_overflow;

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

static void log_command_status(const char *operation, sl_status_t status) {
  (void)sl_iostream_printf(SL_IOSTREAM_STDOUT,
                           "cmd=%s status=0x%08" PRIX32 "\r\n", operation,
                           (uint32_t)status);
}

static const char *scan_mode_name(uint8_t mode) {
  return (mode == sl_bt_scanner_scan_mode_active) ? "active" : "passive";
}

static const char *scan_phy_name(uint8_t phy) {
  switch (phy) {
  case sl_bt_scanner_scan_phy_coded:
    return "coded";
  case sl_bt_scanner_scan_phy_1m_and_coded:
    return "both";
  case sl_bt_scanner_scan_phy_1m:
  default:
    return "1m";
  }
}

static const char *discover_mode_name(uint8_t discover) {
  switch (discover) {
  case sl_bt_scanner_discover_limited:
    return "limited";
  case sl_bt_scanner_discover_generic:
    return "generic";
  case sl_bt_scanner_discover_observation:
  default:
    return "observation";
  }
}

static sl_status_t configure_scanner(void) {
  return sl_bt_scanner_set_parameters_and_filter(
      runtime_state.scan.mode, runtime_state.scan.interval,
      runtime_state.scan.window, runtime_state.scan.flags,
      runtime_state.scan.filter);
}

static sl_status_t start_scanner(void) {
  sl_status_t status;

  if (runtime_state.scanning) {
    return SL_STATUS_ALREADY_EXISTS;
  }

  status = configure_scanner();
  if (status != SL_STATUS_OK) {
    return status;
  }

  status =
      sl_bt_scanner_start(runtime_state.scan.phy, runtime_state.scan.discover);
  if (status == SL_STATUS_OK) {
    runtime_state.scanning = true;
  }
  return status;
}

static sl_status_t stop_scanner(void) {
  sl_status_t status;

  if (!runtime_state.scanning) {
    return SL_STATUS_NOT_FOUND;
  }

  status = sl_bt_scanner_stop();
  if (status == SL_STATUS_OK) {
    runtime_state.scanning = false;
  }
  return status;
}

static void log_identity(void) {
  bd_addr address;
  uint8_t address_type;
  const sl_status_t status =
      sl_bt_gap_get_identity_address(&address, &address_type);

  if (status != SL_STATUS_OK) {
    log_command_status("identity-get", status);
    return;
  }

  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "identity addr=%02X:%02X:%02X:%02X:%02X:%02X type=%u\r\n",
      address.addr[5], address.addr[4], address.addr[3], address.addr[2],
      address.addr[1], address.addr[0], address_type);
}

static void log_tx_power(void) {
  int16_t support_min;
  int16_t support_max;
  int16_t set_min;
  int16_t set_max;
  int16_t rf_path_gain;
  const sl_status_t status = sl_bt_system_get_tx_power_setting(
      &support_min, &support_max, &set_min, &set_max, &rf_path_gain);

  if (status != SL_STATUS_OK) {
    log_command_status("tx-get", status);
    return;
  }

  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "tx support=%d..%d set=%d..%d rf_path_gain=%d units=0.1dBm\r\n",
      support_min, support_max, set_min, set_max, rf_path_gain);
}

static void log_status(void) {
  const scan_config_t *config = &runtime_state.scan;

  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "board=BRD4181A target=EFR32MG21A010F1024IM32 sdk=2025.6.3 "
      "rtt=nonblocking em1=required stack=%s\r\n",
      runtime_state.stack_ready ? "ready" : "not-ready");
  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "scan=%s mode=%s interval=%u window=%u phy=%s discover=%s "
      "flags=0x%" PRIX32 " filter=%u logs=observations:%s,summaries:%s\r\n",
      runtime_state.scanning ? "on" : "off", scan_mode_name(config->mode),
      config->interval, config->window, scan_phy_name(config->phy),
      discover_mode_name(config->discover), config->flags, config->filter,
      runtime_state.log_observations ? "on" : "off",
      runtime_state.log_summaries ? "on" : "off");
  log_identity();
  log_tx_power();
}

static void print_help(void) {
  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "commands:\r\n"
      "  status\r\n"
      "  scan start|stop\r\n"
      "  scan set <passive|active> <interval> <window> "
      "<1m|coded|both> <limited|generic|observation> [flags] [filter]\r\n"
      "  tx get | tx set <min_x10> <max_x10>\r\n"
      "  identity get\r\n"
      "  log set <observations|summaries> <on|off>\r\n"
      "values: scan times use 0.625ms units; tx uses 0.1dBm; flags/filter "
      "are numeric\r\n");
}

static void log_observation(const scan_observation_result_t *result,
                            uint32_t now_ms) {
  if (!runtime_state.log_observations) {
    return;
  }

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

  if (!runtime_state.log_summaries) {
    return;
  }

  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "summary t=%" PRIu32 " reports=%" PRIu32 " discoveries=%" PRIu32
      " active=%" PRIu32 " evictions=%" PRIu32 " suppressed=%" PRIu32
      " malformed=%" PRIu32 "\r\n",
      now_ms, stats.total_reports, stats.discoveries, stats.active_devices,
      stats.cache_evictions, stats.suppressed_logs, stats.malformed_payloads);
}

static void handle_scan_configuration(const rtt_command_t *command) {
  const scan_config_t old_config = runtime_state.scan;
  const bool restart = runtime_state.scanning;
  sl_status_t status;

  if (restart) {
    status = stop_scanner();
    if (status != SL_STATUS_OK) {
      log_command_status("scan-stop", status);
      return;
    }
  }

  runtime_state.scan.mode = (uint8_t)command->arguments[0];
  runtime_state.scan.interval = (uint16_t)command->arguments[1];
  runtime_state.scan.window = (uint16_t)command->arguments[2];
  runtime_state.scan.phy = (uint8_t)command->arguments[3];
  runtime_state.scan.discover = (uint8_t)command->arguments[4];
  runtime_state.scan.flags = (uint32_t)command->arguments[5];
  runtime_state.scan.filter = (uint8_t)command->arguments[6];

  status = configure_scanner();
  if (status != SL_STATUS_OK) {
    runtime_state.scan = old_config;
    (void)configure_scanner();
    log_command_status("scan-config", status);
    if (restart) {
      status = start_scanner();
      log_command_status("scan-restore", status);
    }
    return;
  }

  if (restart) {
    status = start_scanner();
    if (status != SL_STATUS_OK) {
      log_command_status("scan-start", status);
      return;
    }
  }

  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "scan configured mode=%s interval=%u window=%u phy=%s discover=%s "
      "flags=0x%" PRIX32 " filter=%u\r\n",
      scan_mode_name(runtime_state.scan.mode), runtime_state.scan.interval,
      runtime_state.scan.window, scan_phy_name(runtime_state.scan.phy),
      discover_mode_name(runtime_state.scan.discover), runtime_state.scan.flags,
      runtime_state.scan.filter);
}

static void handle_command(const char *line, size_t line_length) {
  rtt_command_t command;
  const rtt_command_parse_result_t parse_result =
      rtt_command_parse(line, line_length, &command);
  sl_status_t status;

  if (parse_result != RTT_COMMAND_PARSE_OK) {
    (void)sl_iostream_printf(SL_IOSTREAM_STDOUT, "error=parse-%s\r\n",
                             rtt_command_parse_result_name(parse_result));
    return;
  }
  if (!runtime_state.stack_ready) {
    (void)sl_iostream_printf(SL_IOSTREAM_STDOUT, "error=stack-not-ready\r\n");
    return;
  }

  switch (command.type) {
  case RTT_COMMAND_HELP:
    print_help();
    break;
  case RTT_COMMAND_STATUS:
    log_status();
    break;
  case RTT_COMMAND_SCAN_START:
    status = start_scanner();
    log_command_status("scan-start", status);
    break;
  case RTT_COMMAND_SCAN_STOP:
    status = stop_scanner();
    log_command_status("scan-stop", status);
    break;
  case RTT_COMMAND_SCAN_SET:
    handle_scan_configuration(&command);
    break;
  case RTT_COMMAND_TX_GET:
    log_tx_power();
    break;
  case RTT_COMMAND_TX_SET: {
    int16_t set_min;
    int16_t set_max;
    if (runtime_state.scanning) {
      (void)sl_iostream_printf(SL_IOSTREAM_STDOUT,
                               "error=stop-scan-before-tx-power\r\n");
      break;
    }
    status = sl_bt_system_set_tx_power((int16_t)command.arguments[0],
                                       (int16_t)command.arguments[1], &set_min,
                                       &set_max);
    log_command_status("tx-set", status);
    if (status == SL_STATUS_OK) {
      (void)sl_iostream_printf(SL_IOSTREAM_STDOUT,
                               "tx selected=%d..%d units=0.1dBm\r\n", set_min,
                               set_max);
    }
    break;
  }
  case RTT_COMMAND_IDENTITY_GET:
    log_identity();
    break;
  case RTT_COMMAND_LOG_SET:
    if (command.arguments[0] == 0) {
      runtime_state.log_observations = command.arguments[1] != 0;
    } else {
      runtime_state.log_summaries = command.arguments[1] != 0;
    }
    (void)sl_iostream_printf(SL_IOSTREAM_STDOUT, "log %s=%s\r\n",
                             (command.arguments[0] == 0) ? "observations"
                                                         : "summaries",
                             (command.arguments[1] != 0) ? "on" : "off");
    break;
  case RTT_COMMAND_INVALID:
  default:
    (void)sl_iostream_printf(SL_IOSTREAM_STDOUT, "error=invalid-command\r\n");
    break;
  }
}

static void process_rtt_input(void) {
  uint8_t input[RTT_READ_CHUNK_SIZE];
  size_t bytes_read = 0U;

  if (sl_iostream_read(SL_IOSTREAM_STDIN, input, sizeof(input), &bytes_read) !=
      SL_STATUS_OK) {
    return;
  }

  for (size_t index = 0U; index < bytes_read; ++index) {
    const uint8_t character = input[index];

    if ((character == '\r') || (character == '\n')) {
      if (rtt_command_line_overflow) {
        (void)sl_iostream_printf(SL_IOSTREAM_STDOUT,
                                 "error=command-too-long\r\n");
      } else if (rtt_command_line_length != 0U) {
        handle_command(rtt_command_line, rtt_command_line_length);
      }
      rtt_command_line_length = 0U;
      rtt_command_line_overflow = false;
      continue;
    }

    if ((character == '\b') || (character == 0x7fU)) {
      if (rtt_command_line_length != 0U) {
        --rtt_command_line_length;
      }
      continue;
    }
    if ((character < 0x20U) || (character > 0x7eU)) {
      continue;
    }
    if (rtt_command_line_length < (RTT_COMMAND_LINE_CAPACITY - 1U)) {
      rtt_command_line[rtt_command_line_length++] = (char)character;
    } else {
      rtt_command_line_overflow = true;
    }
  }
}

void app_init(void) {
  scan_tracker_init(&scan_tracker, uptime_ms());

  // RTT lives in target RAM and is not readable by the debug probe in EM2.
  // Keep EM1 as the lowest sleep mode for the lifetime of this logging build.
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

  (void)sl_iostream_printf(
      SL_IOSTREAM_STDOUT,
      "BLE RTT control initialized; waiting for stack boot\r\n");
}

void app_process_action(void) {
  const uint32_t now_ms = uptime_ms();

  process_rtt_input();
  if (scan_tracker_summary_due(&scan_tracker, now_ms)) {
    log_summary(now_ms);
  }
}

void sl_bt_on_event(sl_bt_msg_t *event) {
  sl_status_t status;

  switch (SL_BT_MSG_ID(event->header)) {
  case sl_bt_evt_system_boot_id:
    runtime_state.stack_ready = true;
    status = start_scanner();
    app_assert_status(status);

    (void)sl_iostream_printf(
        SL_IOSTREAM_STDOUT,
        "BLE scan started: mode=%s interval=%u window=%u phy=%s "
        "discover=%s\r\n",
        scan_mode_name(runtime_state.scan.mode), runtime_state.scan.interval,
        runtime_state.scan.window, scan_phy_name(runtime_state.scan.phy),
        discover_mode_name(runtime_state.scan.discover));
    (void)sl_iostream_printf(SL_IOSTREAM_STDOUT,
                             "type 'help' for RTT control commands\r\n");
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

  case sl_bt_evt_scanner_extended_advertisement_report_id: {
    const sl_bt_evt_scanner_extended_advertisement_report_t *report =
        &event->data.evt_scanner_extended_advertisement_report;
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
