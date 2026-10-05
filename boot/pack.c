/* Host-only build transform for our kernel bytes; never linked into the guest.
 * Format: tag <128 -> tag+1 literal bytes; tag >=128 -> (tag&127)+3
 * bytes copied from a following little-endian 16-bit backward distance.
 * Decoder, bounds checks and overlapping copies are authored in stage2.asm. */
#include <stdint.h>
#include <stdio.h>
static uint8_t input[524289], token[524288];
static uint16_t distance[524288];
static uint32_t cost[524289];
int main(void) {
  size_t size = fread(input, 1, sizeof(input), stdin);
  if (ferror(stdin) || !size || size == sizeof(input))
    return 1;
  /* Every legal literal and match length is considered. The cheapest suffix
   * gives the smallest stream in this format, independent of greedy choices. */
  for (size_t cursor = size; cursor-- > 0;) {
    uint32_t best_cost = UINT32_MAX;
    size_t remaining = size - cursor;
    for (size_t length = 1; length <= 128 && length <= remaining; length++) {
      uint32_t candidate = 1 + (uint32_t)length + cost[cursor + length];
      if (candidate < best_cost) {
        best_cost = candidate;
        token[cursor] = (uint8_t)(length - 1);
      }
    }
    size_t best = 0, offset_best = 0;
    size_t limit = cursor < 65535 ? cursor : 65535;
    for (size_t offset = 1; offset <= limit; offset++) {
      if (input[cursor] != input[cursor - offset])
        continue;
      size_t length = 1;
      while (length < 130 && length < remaining &&
             input[cursor + length] == input[cursor + length - offset])
        length++;
      if (length > best) {
        best = length;
        offset_best = offset;
      }
      if (best == 130)
        break;
    }
    for (size_t length = 3; length <= best; length++) {
      uint32_t candidate = 3 + cost[cursor + length];
      if (candidate < best_cost) {
        best_cost = candidate;
        token[cursor] = (uint8_t)(128 | (length - 3));
        distance[cursor] = (uint16_t)offset_best;
      }
    }
    cost[cursor] = best_cost;
  }
  for (size_t cursor = 0; cursor < size;) {
    uint8_t tag = token[cursor];
    putchar(tag);
    if (tag < 128) {
      size_t count = (size_t)tag + 1;
      fwrite(input + cursor, 1, count, stdout);
      cursor += count;
    } else {
      putchar(distance[cursor] & 255);
      putchar(distance[cursor] >> 8);
      cursor += (tag & 127) + 3;
    }
  }
  return ferror(stdout) || fflush(stdout) ? 1 : 0;
}
