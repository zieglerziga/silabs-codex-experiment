#include "scan_tracker.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned int failures;

#define EXPECT_TRUE(condition)                                                 \
  do {                                                                         \
    if (!(condition)) {                                                        \
      (void)printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);        \
      failures++;                                                              \
    }                                                                          \
  } while (0)

#define EXPECT_EQ_UINT(expected, actual)                                       \
  do {                                                                         \
    const unsigned long expected_value = (unsigned long)(expected);            \
    const unsigned long actual_value = (unsigned long)(actual);                \
    if (expected_value != actual_value) {                                      \
      (void)printf("FAIL %s:%d: expected %lu, got %lu\n", __FILE__, __LINE__,  \
                   expected_value, actual_value);                              \
      failures++;                                                              \
    }                                                                          \
  } while (0)

static scan_observation_t observation_for(uint8_t identity, int8_t rssi,
                                          const uint8_t *payload,
                                          size_t payload_length) {
  scan_observation_t observation = {
      .address = {identity, 2U, 3U, 4U, 5U, 6U},
      .address_type = 0U,
      .rssi = rssi,
      .payload = payload,
      .payload_length = payload_length,
  };
  return observation;
}

static void test_first_sighting_and_suppression(void) {
  static const uint8_t payload[] = {5U, 0x09U, 'T', 'e', 's', 't'};
  scan_tracker_t tracker;
  scan_tracker_init(&tracker, 100U);
  const scan_observation_t observation =
      observation_for(1U, -50, payload, sizeof(payload));

  const scan_observation_result_t first =
      scan_tracker_observe(&tracker, &observation, 100U);
  const scan_observation_result_t duplicate =
      scan_tracker_observe(&tracker, &observation, 200U);

  EXPECT_EQ_UINT(SCAN_LOG_FIRST_SEEN, first.reason);
  EXPECT_TRUE(strcmp(first.device.name, "Test") == 0);
  EXPECT_EQ_UINT(SCAN_LOG_NONE, duplicate.reason);
  EXPECT_EQ_UINT(2U, duplicate.device.report_count);
  EXPECT_EQ_UINT(1U, tracker.stats.suppressed_logs);
}

static void test_name_and_rssi_changes_are_rate_limited(void) {
  static const uint8_t old_name[] = {4U, 0x09U, 'O', 'l', 'd'};
  static const uint8_t new_name[] = {4U, 0x09U, 'N', 'e', 'w'};
  scan_tracker_t tracker;
  scan_tracker_init(&tracker, 0U);
  scan_observation_t observation =
      observation_for(2U, -50, old_name, sizeof(old_name));

  (void)scan_tracker_observe(&tracker, &observation, 0U);
  observation.payload = new_name;
  observation.payload_length = sizeof(new_name);
  EXPECT_EQ_UINT(SCAN_LOG_NONE,
                 scan_tracker_observe(&tracker, &observation, 500U).reason);
  EXPECT_EQ_UINT(SCAN_LOG_NAME_CHANGED,
                 scan_tracker_observe(&tracker, &observation, 1000U).reason);

  observation.rssi = -70;
  EXPECT_EQ_UINT(SCAN_LOG_NONE,
                 scan_tracker_observe(&tracker, &observation, 1500U).reason);
  EXPECT_EQ_UINT(SCAN_LOG_RSSI_CHANGED,
                 scan_tracker_observe(&tracker, &observation, 2000U).reason);
}

static void test_complete_name_wins_and_malformed_data_is_counted(void) {
  static const uint8_t names[] = {
      4U, 0x08U, 'S', 'h', 'y', 5U, 0x09U, 'F', 'u', 'l', 'l',
  };
  static const uint8_t malformed[] = {8U, 0x09U, 'B', 'a', 'd'};
  scan_tracker_t tracker;
  scan_tracker_init(&tracker, 0U);
  scan_observation_t observation =
      observation_for(3U, -40, names, sizeof(names));

  const scan_observation_result_t named =
      scan_tracker_observe(&tracker, &observation, 0U);
  EXPECT_TRUE(strcmp(named.device.name, "Full") == 0);

  observation.address[0] = 4U;
  observation.payload = malformed;
  observation.payload_length = sizeof(malformed);
  const scan_observation_result_t bad =
      scan_tracker_observe(&tracker, &observation, 1U);
  EXPECT_TRUE(bad.malformed_payload);
  EXPECT_EQ_UINT(1U, tracker.stats.malformed_payloads);
}

static void test_cache_evicts_the_oldest_device(void) {
  scan_tracker_t tracker;
  scan_tracker_init(&tracker, 0U);

  for (uint8_t index = 0U; index < SCAN_TRACKER_CAPACITY; ++index) {
    const scan_observation_t observation =
        observation_for(index, -60, NULL, 0U);
    (void)scan_tracker_observe(&tracker, &observation, (uint32_t)index * 10U);
  }

  const scan_observation_t overflow = observation_for(0xf0U, -70, NULL, 0U);
  const scan_observation_result_t result =
      scan_tracker_observe(&tracker, &overflow, 1000U);

  EXPECT_EQ_UINT(SCAN_LOG_FIRST_SEEN, result.reason);
  EXPECT_EQ_UINT(SCAN_TRACKER_CAPACITY, tracker.stats.active_devices);
  EXPECT_EQ_UINT(1U, tracker.stats.cache_evictions);
  EXPECT_EQ_UINT(SCAN_TRACKER_CAPACITY + 1U, tracker.stats.discoveries);
}

static void test_timers_handle_uint32_wrap(void) {
  scan_tracker_t tracker;
  const uint32_t start = UINT32_MAX - 100U;
  scan_tracker_init(&tracker, start);
  const scan_observation_t observation = observation_for(5U, -30, NULL, 0U);

  (void)scan_tracker_observe(&tracker, &observation, start);
  EXPECT_EQ_UINT(SCAN_LOG_RSSI_CHANGED,
                 scan_tracker_observe(&tracker,
                                      &(scan_observation_t){
                                          .address = {5U, 2U, 3U, 4U, 5U, 6U},
                                          .address_type = 0U,
                                          .rssi = -50,
                                          .payload = NULL,
                                          .payload_length = 0U,
                                      },
                                      1000U)
                     .reason);

  EXPECT_TRUE(!scan_tracker_summary_due(&tracker, 1000U));
  EXPECT_TRUE(scan_tracker_summary_due(&tracker, 30000U));
}

static void test_summary_snapshot_keeps_cumulative_counters(void) {
  scan_tracker_t tracker;
  scan_tracker_init(&tracker, 0U);
  const scan_observation_t observation = observation_for(6U, -45, NULL, 0U);
  (void)scan_tracker_observe(&tracker, &observation, 1U);

  const scan_tracker_stats_t first =
      scan_tracker_snapshot_summary(&tracker, 30000U);
  const scan_tracker_stats_t second =
      scan_tracker_snapshot_summary(&tracker, 60000U);

  EXPECT_EQ_UINT(1U, first.total_reports);
  EXPECT_EQ_UINT(1U, second.total_reports);
  EXPECT_TRUE(!scan_tracker_summary_due(&tracker, 60000U));
  EXPECT_TRUE(scan_tracker_summary_due(&tracker, 90000U));
}

int main(void) {
  test_first_sighting_and_suppression();
  test_name_and_rssi_changes_are_rate_limited();
  test_complete_name_wins_and_malformed_data_is_counted();
  test_cache_evicts_the_oldest_device();
  test_timers_handle_uint32_wrap();
  test_summary_snapshot_keeps_cumulative_counters();

  if (failures == 0U) {
    (void)puts("scan_tracker_tests: all tests passed");
    return 0;
  }

  (void)printf("scan_tracker_tests: %u failure(s)\n", failures);
  return 1;
}
