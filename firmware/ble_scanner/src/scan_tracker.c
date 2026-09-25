#include "scan_tracker.h"

#include <limits.h>
#include <string.h>

#define AD_TYPE_SHORTENED_LOCAL_NAME 0x08U
#define AD_TYPE_COMPLETE_LOCAL_NAME 0x09U

typedef struct {
  char value[SCAN_TRACKER_NAME_CAPACITY];
  bool present;
  bool complete;
  bool malformed;
} parsed_name_t;

static uint32_t elapsed_ms(uint32_t now_ms, uint32_t then_ms) {
  return now_ms - then_ms;
}

static bool deadline_reached(uint32_t now_ms, uint32_t deadline_ms) {
  return (now_ms - deadline_ms) < (UINT32_MAX / 2U);
}

static int rssi_distance(int8_t lhs, int8_t rhs) {
  int distance = (int)lhs - (int)rhs;
  return (distance < 0) ? -distance : distance;
}

static parsed_name_t parse_local_name(const uint8_t *payload,
                                      size_t payload_length) {
  parsed_name_t result = {0};
  size_t offset = 0U;

  if ((payload == NULL) && (payload_length != 0U)) {
    result.malformed = true;
    return result;
  }

  while (offset < payload_length) {
    const size_t remaining = payload_length - offset;
    const uint8_t field_length = payload[offset];

    if (field_length == 0U) {
      break;
    }
    if ((size_t)field_length >= remaining) {
      result.malformed = true;
      break;
    }

    const uint8_t field_type = payload[offset + 1U];
    const bool is_complete = field_type == AD_TYPE_COMPLETE_LOCAL_NAME;
    const bool is_shortened = field_type == AD_TYPE_SHORTENED_LOCAL_NAME;

    if ((is_complete || is_shortened) && (!result.complete || is_complete)) {
      const size_t source_length = (size_t)field_length - 1U;
      const size_t copy_length =
          (source_length < (SCAN_TRACKER_NAME_CAPACITY - 1U))
              ? source_length
              : (SCAN_TRACKER_NAME_CAPACITY - 1U);

      for (size_t index = 0U; index < copy_length; ++index) {
        const uint8_t character = payload[offset + 2U + index];
        result.value[index] = ((character >= 0x20U) && (character <= 0x7eU))
                                  ? (char)character
                                  : '.';
      }
      result.value[copy_length] = '\0';
      result.present = true;
      result.complete = is_complete;
    }

    offset += (size_t)field_length + 1U;
  }

  return result;
}

static bool entry_matches(const scan_tracker_entry_t *entry,
                          const scan_observation_t *observation) {
  return entry->occupied &&
         (entry->address_type == observation->address_type) &&
         (memcmp(entry->address, observation->address,
                 SCAN_TRACKER_ADDRESS_SIZE) == 0);
}

static size_t select_entry(scan_tracker_t *tracker,
                           const scan_observation_t *observation,
                           uint32_t now_ms, bool *is_new) {
  size_t empty_index = SCAN_TRACKER_CAPACITY;
  size_t oldest_index = 0U;
  uint32_t oldest_age = 0U;

  for (size_t index = 0U; index < SCAN_TRACKER_CAPACITY; ++index) {
    scan_tracker_entry_t *entry = &tracker->entries[index];

    if (entry_matches(entry, observation)) {
      *is_new = false;
      return index;
    }
    if (!entry->occupied) {
      if (empty_index == SCAN_TRACKER_CAPACITY) {
        empty_index = index;
      }
      continue;
    }

    const uint32_t age = elapsed_ms(now_ms, entry->last_seen_ms);
    if (age > oldest_age) {
      oldest_age = age;
      oldest_index = index;
    }
  }

  *is_new = true;
  if (empty_index != SCAN_TRACKER_CAPACITY) {
    tracker->stats.active_devices++;
    return empty_index;
  }

  tracker->stats.cache_evictions++;
  return oldest_index;
}

static void copy_snapshot(scan_device_snapshot_t *snapshot,
                          const scan_tracker_entry_t *entry) {
  memcpy(snapshot->address, entry->address, SCAN_TRACKER_ADDRESS_SIZE);
  snapshot->address_type = entry->address_type;
  snapshot->rssi = entry->rssi;
  memcpy(snapshot->name, entry->name, sizeof(snapshot->name));
  snapshot->report_count = entry->report_count;
}

void scan_tracker_init(scan_tracker_t *tracker, uint32_t now_ms) {
  if (tracker == NULL) {
    return;
  }

  memset(tracker, 0, sizeof(*tracker));
  tracker->next_summary_ms = now_ms + SCAN_TRACKER_SUMMARY_INTERVAL_MS;
}

scan_observation_result_t
scan_tracker_observe(scan_tracker_t *tracker,
                     const scan_observation_t *observation, uint32_t now_ms) {
  scan_observation_result_t result = {0};
  bool is_new = false;

  if ((tracker == NULL) || (observation == NULL)) {
    return result;
  }

  tracker->stats.total_reports++;
  const parsed_name_t parsed_name =
      parse_local_name(observation->payload, observation->payload_length);
  if (parsed_name.malformed) {
    tracker->stats.malformed_payloads++;
    result.malformed_payload = true;
  }

  const size_t entry_index =
      select_entry(tracker, observation, now_ms, &is_new);
  scan_tracker_entry_t *entry = &tracker->entries[entry_index];

  if (is_new) {
    memset(entry, 0, sizeof(*entry));
    entry->occupied = true;
    memcpy(entry->address, observation->address, SCAN_TRACKER_ADDRESS_SIZE);
    entry->address_type = observation->address_type;
    entry->rssi = observation->rssi;
    entry->last_logged_rssi = observation->rssi;
    entry->last_seen_ms = now_ms;
    entry->last_logged_ms = now_ms;
    entry->report_count = 1U;
    if (parsed_name.present) {
      memcpy(entry->name, parsed_name.value, sizeof(entry->name));
      memcpy(entry->last_logged_name, parsed_name.value,
             sizeof(entry->last_logged_name));
    }
    tracker->stats.discoveries++;
    result.reason = SCAN_LOG_FIRST_SEEN;
  } else {
    const uint32_t since_last_log = elapsed_ms(now_ms, entry->last_logged_ms);

    entry->rssi = observation->rssi;
    entry->last_seen_ms = now_ms;
    if (entry->report_count != UINT32_MAX) {
      entry->report_count++;
    }
    if (parsed_name.present) {
      memcpy(entry->name, parsed_name.value, sizeof(entry->name));
    }

    const bool name_changed = strcmp(entry->name, entry->last_logged_name) != 0;
    const bool rssi_changed =
        rssi_distance(entry->rssi, entry->last_logged_rssi) >=
        SCAN_TRACKER_RSSI_DELTA_DB;
    const bool rate_limit_expired =
        since_last_log >= SCAN_TRACKER_MIN_LOG_INTERVAL_MS;

    if (rate_limit_expired && name_changed) {
      result.reason = SCAN_LOG_NAME_CHANGED;
    } else if (rate_limit_expired && rssi_changed) {
      result.reason = SCAN_LOG_RSSI_CHANGED;
    } else if (since_last_log >= SCAN_TRACKER_REFRESH_INTERVAL_MS) {
      result.reason = SCAN_LOG_PERIODIC_REFRESH;
    } else {
      tracker->stats.suppressed_logs++;
    }

    if (result.reason != SCAN_LOG_NONE) {
      entry->last_logged_ms = now_ms;
      entry->last_logged_rssi = entry->rssi;
      memcpy(entry->last_logged_name, entry->name,
             sizeof(entry->last_logged_name));
    }
  }

  copy_snapshot(&result.device, entry);
  return result;
}

bool scan_tracker_summary_due(const scan_tracker_t *tracker, uint32_t now_ms) {
  return (tracker != NULL) &&
         deadline_reached(now_ms, tracker->next_summary_ms);
}

scan_tracker_stats_t scan_tracker_snapshot_summary(scan_tracker_t *tracker,
                                                   uint32_t now_ms) {
  scan_tracker_stats_t empty = {0};

  if (tracker == NULL) {
    return empty;
  }

  tracker->next_summary_ms = now_ms + SCAN_TRACKER_SUMMARY_INTERVAL_MS;
  return tracker->stats;
}
