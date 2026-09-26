#ifndef RTT_COMMAND_H
#define RTT_COMMAND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTT_COMMAND_ARGUMENT_COUNT 7U
#define RTT_COMMAND_TOKEN_COUNT 9U
#define RTT_COMMAND_SUPPORTED_SCAN_FLAGS 0x1U

typedef enum {
  RTT_COMMAND_PARSE_OK = 0,
  RTT_COMMAND_PARSE_EMPTY,
  RTT_COMMAND_PARSE_UNKNOWN,
  RTT_COMMAND_PARSE_SYNTAX,
  RTT_COMMAND_PARSE_RANGE,
  RTT_COMMAND_PARSE_TOO_MANY_TOKENS
} rtt_command_parse_result_t;

typedef enum {
  RTT_COMMAND_INVALID = 0,
  RTT_COMMAND_HELP,
  RTT_COMMAND_STATUS,
  RTT_COMMAND_SCAN_START,
  RTT_COMMAND_SCAN_STOP,
  RTT_COMMAND_SCAN_SET,
  RTT_COMMAND_TX_GET,
  RTT_COMMAND_TX_SET,
  RTT_COMMAND_IDENTITY_GET,
  RTT_COMMAND_LOG_SET
} rtt_command_type_t;

typedef struct {
  rtt_command_type_t type;
  int64_t arguments[RTT_COMMAND_ARGUMENT_COUNT];
} rtt_command_t;

typedef struct {
  const char *data;
  size_t length;
} rtt_command_token_t;

static inline bool rtt_command_token_equals(rtt_command_token_t token,
                                            const char *literal) {
  const size_t literal_length = strlen(literal);
  return (token.length == literal_length) &&
         (memcmp(token.data, literal, literal_length) == 0);
}

static inline bool rtt_command_parse_unsigned(rtt_command_token_t token,
                                              uint32_t *value) {
  size_t index = 0U;
  uint32_t base = 10U;
  uint32_t parsed = 0U;
  bool digit_seen = false;

  if ((token.data == NULL) || (value == NULL) || (token.length == 0U)) {
    return false;
  }

  if ((token.length >= 2U) && (token.data[0] == '0') &&
      ((token.data[1] == 'x') || (token.data[1] == 'X'))) {
    base = 16U;
    index = 2U;
  }

  for (; index < token.length; ++index) {
    const char character = token.data[index];
    uint32_t digit;

    if ((character >= '0') && (character <= '9')) {
      digit = (uint32_t)(character - '0');
    } else if ((base == 16U) && (character >= 'a') && (character <= 'f')) {
      digit = (uint32_t)(character - 'a') + 10U;
    } else if ((base == 16U) && (character >= 'A') && (character <= 'F')) {
      digit = (uint32_t)(character - 'A') + 10U;
    } else {
      return false;
    }

    if (digit >= base) {
      return false;
    }
    if ((parsed > (UINT32_MAX - digit) / base)) {
      return false;
    }

    parsed = (parsed * base) + digit;
    digit_seen = true;
  }

  if (!digit_seen) {
    return false;
  }

  *value = parsed;
  return true;
}

static inline bool rtt_command_parse_signed(rtt_command_token_t token,
                                            int32_t *value) {
  bool negative = false;
  uint32_t magnitude;

  if ((token.data == NULL) || (value == NULL) || (token.length == 0U)) {
    return false;
  }

  if ((token.data[0] == '-') || (token.data[0] == '+')) {
    negative = token.data[0] == '-';
    token.data++;
    token.length--;
  }

  if (!rtt_command_parse_unsigned(token, &magnitude)) {
    return false;
  }

  if (negative) {
    if (magnitude > (uint32_t)INT32_MAX + 1U) {
      return false;
    }
    *value = (magnitude == (uint32_t)INT32_MAX + 1U) ? INT32_MIN
                                                     : -(int32_t)magnitude;
  } else {
    if (magnitude > (uint32_t)INT32_MAX) {
      return false;
    }
    *value = (int32_t)magnitude;
  }

  return true;
}

static inline bool
rtt_command_parse_named_u32(rtt_command_token_t token, const char *first_name,
                            uint32_t first_value, const char *second_name,
                            uint32_t second_value, uint32_t *value) {
  if (rtt_command_token_equals(token, first_name)) {
    *value = first_value;
    return true;
  }
  if (rtt_command_token_equals(token, second_name)) {
    *value = second_value;
    return true;
  }
  return rtt_command_parse_unsigned(token, value);
}

static inline bool rtt_command_parse_scan_mode(rtt_command_token_t token,
                                               uint32_t *value) {
  if (!rtt_command_parse_named_u32(token, "passive", 0U, "active", 1U, value)) {
    return false;
  }
  return *value <= 1U;
}

static inline bool rtt_command_parse_scan_phy(rtt_command_token_t token,
                                              uint32_t *value) {
  if (rtt_command_token_equals(token, "1m")) {
    *value = 1U;
    return true;
  }
  if (rtt_command_token_equals(token, "coded")) {
    *value = 4U;
    return true;
  }
  if (rtt_command_token_equals(token, "both")) {
    *value = 5U;
    return true;
  }

  if (!rtt_command_parse_unsigned(token, value)) {
    return false;
  }
  return (*value == 1U) || (*value == 4U) || (*value == 5U);
}

static inline bool rtt_command_parse_discover_mode(rtt_command_token_t token,
                                                   uint32_t *value) {
  if (rtt_command_token_equals(token, "limited")) {
    *value = 0U;
    return true;
  }
  if (rtt_command_token_equals(token, "generic")) {
    *value = 1U;
    return true;
  }
  if (rtt_command_token_equals(token, "observation")) {
    *value = 2U;
    return true;
  }

  if (!rtt_command_parse_unsigned(token, value)) {
    return false;
  }
  return *value <= 2U;
}

static inline bool rtt_command_parse_on_off(rtt_command_token_t token,
                                            uint32_t *value) {
  if (rtt_command_token_equals(token, "on")) {
    *value = 1U;
    return true;
  }
  if (rtt_command_token_equals(token, "off")) {
    *value = 0U;
    return true;
  }

  if (!rtt_command_parse_unsigned(token, value)) {
    return false;
  }
  return *value <= 1U;
}

static inline rtt_command_parse_result_t
rtt_command_parse(const char *line, size_t line_length,
                  rtt_command_t *command) {
  rtt_command_token_t tokens[RTT_COMMAND_TOKEN_COUNT];
  size_t token_count = 0U;
  size_t index = 0U;

  if ((line == NULL) || (command == NULL)) {
    return RTT_COMMAND_PARSE_SYNTAX;
  }
  memset(command, 0, sizeof(*command));

  while (index < line_length) {
    while ((index < line_length) &&
           ((line[index] == ' ') || (line[index] == '\t') ||
            (line[index] == '\r') || (line[index] == '\n'))) {
      ++index;
    }
    if (index == line_length) {
      break;
    }
    if (token_count == RTT_COMMAND_TOKEN_COUNT) {
      return RTT_COMMAND_PARSE_TOO_MANY_TOKENS;
    }

    const size_t token_start = index;
    tokens[token_count].data = &line[token_start];
    while ((index < line_length) && (line[index] != ' ') &&
           (line[index] != '\t') && (line[index] != '\r') &&
           (line[index] != '\n')) {
      ++index;
    }
    tokens[token_count].length = index - token_start;
    ++token_count;
  }

  if (token_count == 0U) {
    return RTT_COMMAND_PARSE_EMPTY;
  }

  if (rtt_command_token_equals(tokens[0], "help")) {
    command->type = RTT_COMMAND_HELP;
    return (token_count == 1U) ? RTT_COMMAND_PARSE_OK
                               : RTT_COMMAND_PARSE_SYNTAX;
  }
  if (rtt_command_token_equals(tokens[0], "status")) {
    command->type = RTT_COMMAND_STATUS;
    return (token_count == 1U) ? RTT_COMMAND_PARSE_OK
                               : RTT_COMMAND_PARSE_SYNTAX;
  }
  if (rtt_command_token_equals(tokens[0], "identity") && (token_count == 2U) &&
      rtt_command_token_equals(tokens[1], "get")) {
    command->type = RTT_COMMAND_IDENTITY_GET;
    return RTT_COMMAND_PARSE_OK;
  }
  if (rtt_command_token_equals(tokens[0], "tx") && (token_count == 2U) &&
      rtt_command_token_equals(tokens[1], "get")) {
    command->type = RTT_COMMAND_TX_GET;
    return RTT_COMMAND_PARSE_OK;
  }
  if (rtt_command_token_equals(tokens[0], "tx") && (token_count == 4U) &&
      rtt_command_token_equals(tokens[1], "set")) {
    int32_t minimum;
    int32_t maximum;
    if (!rtt_command_parse_signed(tokens[2], &minimum) ||
        !rtt_command_parse_signed(tokens[3], &maximum) ||
        (minimum < INT16_MIN) || (minimum > INT16_MAX) ||
        (maximum < INT16_MIN) || (maximum > INT16_MAX) || (minimum > maximum)) {
      return RTT_COMMAND_PARSE_RANGE;
    }
    command->type = RTT_COMMAND_TX_SET;
    command->arguments[0] = minimum;
    command->arguments[1] = maximum;
    return RTT_COMMAND_PARSE_OK;
  }
  if (rtt_command_token_equals(tokens[0], "scan") && (token_count == 2U) &&
      rtt_command_token_equals(tokens[1], "start")) {
    command->type = RTT_COMMAND_SCAN_START;
    return RTT_COMMAND_PARSE_OK;
  }
  if (rtt_command_token_equals(tokens[0], "scan") && (token_count == 2U) &&
      rtt_command_token_equals(tokens[1], "stop")) {
    command->type = RTT_COMMAND_SCAN_STOP;
    return RTT_COMMAND_PARSE_OK;
  }
  if (rtt_command_token_equals(tokens[0], "scan") && (token_count >= 7U) &&
      (token_count <= 9U) && rtt_command_token_equals(tokens[1], "set")) {
    uint32_t mode;
    uint32_t interval;
    uint32_t window;
    uint32_t phy;
    uint32_t discover;
    uint32_t flags = 0U;
    uint32_t filter = 0U;

    if (!rtt_command_parse_scan_mode(tokens[2], &mode) ||
        !rtt_command_parse_unsigned(tokens[3], &interval) ||
        !rtt_command_parse_unsigned(tokens[4], &window) ||
        !rtt_command_parse_scan_phy(tokens[5], &phy) ||
        !rtt_command_parse_discover_mode(tokens[6], &discover)) {
      return RTT_COMMAND_PARSE_RANGE;
    }
    if ((interval < 4U) || (interval > UINT16_MAX) || (window < 4U) ||
        (window > interval)) {
      return RTT_COMMAND_PARSE_RANGE;
    }
    if ((token_count >= 8U) && !rtt_command_parse_unsigned(tokens[7], &flags)) {
      return RTT_COMMAND_PARSE_RANGE;
    }
    if ((token_count == 9U) &&
        !rtt_command_parse_unsigned(tokens[8], &filter)) {
      return RTT_COMMAND_PARSE_RANGE;
    }
    if (filter > 3U) {
      return RTT_COMMAND_PARSE_RANGE;
    }
    if ((flags & ~RTT_COMMAND_SUPPORTED_SCAN_FLAGS) != 0U) {
      return RTT_COMMAND_PARSE_RANGE;
    }

    command->type = RTT_COMMAND_SCAN_SET;
    command->arguments[0] = mode;
    command->arguments[1] = interval;
    command->arguments[2] = window;
    command->arguments[3] = phy;
    command->arguments[4] = discover;
    command->arguments[5] = flags;
    command->arguments[6] = filter;
    return RTT_COMMAND_PARSE_OK;
  }
  if (rtt_command_token_equals(tokens[0], "log") && (token_count == 4U) &&
      rtt_command_token_equals(tokens[1], "set")) {
    uint32_t enabled;
    const bool observations =
        rtt_command_token_equals(tokens[2], "observations");
    const bool summaries = rtt_command_token_equals(tokens[2], "summaries");
    if ((!observations && !summaries) ||
        !rtt_command_parse_on_off(tokens[3], &enabled)) {
      return RTT_COMMAND_PARSE_SYNTAX;
    }
    command->type = RTT_COMMAND_LOG_SET;
    command->arguments[0] = summaries ? 1U : 0U;
    command->arguments[1] = enabled;
    return RTT_COMMAND_PARSE_OK;
  }

  return RTT_COMMAND_PARSE_UNKNOWN;
}

static inline const char *
rtt_command_parse_result_name(rtt_command_parse_result_t result) {
  switch (result) {
  case RTT_COMMAND_PARSE_EMPTY:
    return "empty";
  case RTT_COMMAND_PARSE_UNKNOWN:
    return "unknown";
  case RTT_COMMAND_PARSE_SYNTAX:
    return "syntax";
  case RTT_COMMAND_PARSE_RANGE:
    return "range";
  case RTT_COMMAND_PARSE_TOO_MANY_TOKENS:
    return "too-many-tokens";
  case RTT_COMMAND_PARSE_OK:
  default:
    return "ok";
  }
}

#ifdef __cplusplus
}
#endif

#endif
