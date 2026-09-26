#include "rtt_command.h"

#include <stdint.h>
#include <stdio.h>

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

static void test_scan_set_with_names_and_hex_flags(void) {
  static const char line[] = "scan set passive 160 80 both observation 0x1 3";
  rtt_command_t command;

  EXPECT_EQ_UINT(RTT_COMMAND_PARSE_OK,
                 rtt_command_parse(line, sizeof(line) - 1U, &command));
  EXPECT_EQ_UINT(RTT_COMMAND_SCAN_SET, command.type);
  EXPECT_EQ_UINT(0U, command.arguments[0]);
  EXPECT_EQ_UINT(160U, command.arguments[1]);
  EXPECT_EQ_UINT(80U, command.arguments[2]);
  EXPECT_EQ_UINT(5U, command.arguments[3]);
  EXPECT_EQ_UINT(2U, command.arguments[4]);
  EXPECT_EQ_UINT(1U, command.arguments[5]);
  EXPECT_EQ_UINT(3U, command.arguments[6]);
}

static void test_tx_power_and_simple_commands(void) {
  static const char tx_line[] = "tx set -30 80";
  rtt_command_t command;

  EXPECT_EQ_UINT(RTT_COMMAND_PARSE_OK,
                 rtt_command_parse(tx_line, sizeof(tx_line) - 1U, &command));
  EXPECT_EQ_UINT(RTT_COMMAND_TX_SET, command.type);
  EXPECT_EQ_UINT((uint32_t)-30, (uint32_t)command.arguments[0]);
  EXPECT_EQ_UINT(80U, command.arguments[1]);

  EXPECT_EQ_UINT(
      RTT_COMMAND_PARSE_OK,
      rtt_command_parse("identity get", sizeof("identity get") - 1U, &command));
  EXPECT_EQ_UINT(RTT_COMMAND_IDENTITY_GET, command.type);
  EXPECT_EQ_UINT(RTT_COMMAND_PARSE_OK,
                 rtt_command_parse("log set summaries off",
                                   sizeof("log set summaries off") - 1U,
                                   &command));
  EXPECT_EQ_UINT(RTT_COMMAND_LOG_SET, command.type);
  EXPECT_EQ_UINT(1U, command.arguments[0]);
  EXPECT_EQ_UINT(0U, command.arguments[1]);
}

static void test_defaults_and_scan_ranges(void) {
  rtt_command_t command;

  EXPECT_EQ_UINT(
      RTT_COMMAND_PARSE_OK,
      rtt_command_parse("scan set active 40 20 1m generic",
                        sizeof("scan set active 40 20 1m generic") - 1U,
                        &command));
  EXPECT_EQ_UINT(0U, command.arguments[5]);
  EXPECT_EQ_UINT(0U, command.arguments[6]);

  EXPECT_EQ_UINT(
      RTT_COMMAND_PARSE_RANGE,
      rtt_command_parse("scan set passive 40 80 1m generic",
                        sizeof("scan set passive 40 80 1m generic") - 1U,
                        &command));
  EXPECT_EQ_UINT(RTT_COMMAND_PARSE_RANGE,
                 rtt_command_parse("tx set 80 -30",
                                   sizeof("tx set 80 -30") - 1U, &command));
}

static void test_invalid_input_is_rejected(void) {
  static const char too_many[] = "scan set passive 160 80 1m observation 0 0 x";
  rtt_command_t command;

  EXPECT_EQ_UINT(RTT_COMMAND_PARSE_EMPTY,
                 rtt_command_parse(" \t\r\n", 4U, &command));
  EXPECT_EQ_UINT(RTT_COMMAND_PARSE_UNKNOWN,
                 rtt_command_parse("advertise start",
                                   sizeof("advertise start") - 1U, &command));
  EXPECT_EQ_UINT(
      RTT_COMMAND_PARSE_SYNTAX,
      rtt_command_parse("status now", sizeof("status now") - 1U, &command));
  EXPECT_EQ_UINT(RTT_COMMAND_PARSE_TOO_MANY_TOKENS,
                 rtt_command_parse(too_many, sizeof(too_many) - 1U, &command));
}

int main(void) {
  test_scan_set_with_names_and_hex_flags();
  test_tx_power_and_simple_commands();
  test_defaults_and_scan_ranges();
  test_invalid_input_is_rejected();

  if (failures == 0U) {
    (void)puts("rtt_command_tests: all tests passed");
    return 0;
  }

  (void)printf("rtt_command_tests: %u failure(s)\n", failures);
  return 1;
}
