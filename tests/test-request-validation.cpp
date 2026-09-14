#include "../AlpacaRequestValidation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main()
{
  struct ValidCase
  {
    const char *text;
    uint32_t expected;
  };
  const ValidCase valid[] = {
      {"99", 99}, {"999", 999}, {"0", 0}, {"1", 1}, {"00000000000000999", 999}, {"2147483648", UINT32_C(2147483648)}, {"4294967295", UINT32_MAX}};
  for (const auto &test : valid)
  {
    uint32_t out = 123;
    assert(parseAlpacaUint32(test.text, strlen(test.text), out));
    assert(out == test.expected);
  }
  const char *invalid[] = {"", "-1", "+99", "99x", " 99", "99 ", "9.9", "1e2", "0x63",
                           "4294967296", "999999999999999999999999", "\t99", "99\n"};
  for (const char *text : invalid)
  {
    uint32_t out = 123;
    assert(!parseAlpacaUint32(text, strlen(text), out));
    assert(out == 0);
  }
  uint32_t out = 123;
  assert(!parseAlpacaUint32(nullptr, 1, out) && out == 0);
  const char embeddedNull[] = {'9', '9', '\0', '9'};
  assert(!parseAlpacaUint32(embeddedNull, sizeof(embeddedNull), out) && out == 0);
  puts("PASS: decimal IDs 99/999, uint32 limits, zero, leading zeros, malformed text and overflow");
  return 0;
}
