#include <stdlib.h>

int isValidPort(int port) {
  // checks that the input string is a valid port number,
  // returns the port number if it is, returns 0
  if (port >= 1 && port <= 65525)
    return port;
  // Didn't get a port, return a 0
  return 0;
}

int isValidIp4(char *str) {
  // Returns 1 if you have a valid IPv4 Address, 0 if you don't
  int segs = 0;  /* Segment count. */
  int chcnt = 0; /* Character count within segment. */
  int accum = 0; /* Accumulator for segment. */

  /* Catch NULL pointer. */

  if (str == NULL)
    return 0;

  /* Process every character in string. */

  while (*str != '\0') {
    /* Segment changeover. */

    if (*str == '.') {
      /* Must have some digits in segment. */

      if (chcnt == 0)
        return 0;

      /* Limit number of segments. */

      if (++segs == 4)
        return 0;

      /* Reset segment values and restart loop. */

      chcnt = accum = 0;
      str++;
      continue;
    }
    /* Check numeric. */

    if ((*str < '0') || (*str > '9'))
      return 0;

    /* Accumulate and check segment. */

    if ((accum = accum * 10 + *str - '0') > 255)
      return 0;

    /* Advance other segment specific stuff and continue loop. */

    chcnt++;
    str++;
  }

  /* Check enough segments and enough characters in last segment. */

  if (segs != 3)
    return 0;

  if (chcnt == 0)
    return 0;

  /* Address okay. */

  return 1;
}
