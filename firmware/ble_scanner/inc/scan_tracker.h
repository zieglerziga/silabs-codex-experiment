#ifndef SCAN_TRACKER_H
#define SCAN_TRACKER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCAN_TRACKER_CAPACITY 32U
#define SCAN_TRACKER_ADDRESS_SIZE 6U
#define SCAN_TRACKER_NAME_CAPACITY 25U
#define SCAN_TRACKER_MIN_LOG_INTERVAL_MS 1000U
#define SCAN_TRACKER_REFRESH_INTERVAL_MS 10000U
#define SCAN_TRACKER_SUMMARY_INTERVAL_MS 30000U
#define SCAN_TRACKER_RSSI_DELTA_DB 12

typedef enum {
  SCAN_LOG_NONE = 0,
  SCAN_LOG_FIRST_SEEN,
  SCAN_LOG_NAME_CHANGED,
  SCAN_LOG_RSSI_CHANGED,
  SCAN_LOG_PERIODIC_REFRESH
} scan_log_reason_t;

typedef struct {
  uint8_t address[SCAN_TRACKER_ADDRESS_SIZE];
  uint8_t address_type;
  int8_t rssi;
  const uint8_t *payload;
  size_t payload_length;
} scan_observation_t;

typedef struct {
  uint8_t address[SCAN_TRACKER_ADDRESS_SIZE];
  uint8_t address_type;
  int8_t rssi;
  char name[SCAN_TRACKER_NAME_CAPACITY];
  uint32_t report_count;
} scan_device_snapshot_t;

typedef struct {
  scan_log_reason_t reason;
  scan_device_snapshot_t device;
  bool malformed_payload;
} scan_observation_result_t;

typedef struct {
  uint32_t total_reports;
  uint32_t discoveries;
  uint32_t active_devices;
  uint32_t cache_evictions;
  uint32_t suppressed_logs;
  uint32_t malformed_payloads;
} scan_tracker_stats_t;

typedef struct {
  bool occupied;
  uint8_t address[SCAN_TRACKER_ADDRESS_SIZE];
  uint8_t address_type;
  int8_t rssi;
  int8_t last_logged_rssi;
  char name[SCAN_TRACKER_NAME_CAPACITY];
  char last_logged_name[SCAN_TRACKER_NAME_CAPACITY];
  uint32_t last_seen_ms;
  uint32_t last_logged_ms;
  uint32_t report_count;
} scan_tracker_entry_t;

typedef struct {
  scan_tracker_entry_t entries[SCAN_TRACKER_CAPACITY];
  scan_tracker_stats_t stats;
  uint32_t next_summary_ms;
} scan_tracker_t;

void scan_tracker_init(scan_tracker_t *tracker, uint32_t now_ms);

scan_observation_result_t
scan_tracker_observe(scan_tracker_t *tracker,
                     const scan_observation_t *observation, uint32_t now_ms);

bool scan_tracker_summary_due(const scan_tracker_t *tracker, uint32_t now_ms);

scan_tracker_stats_t scan_tracker_take_summary(scan_tracker_t *tracker,
                                               uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
