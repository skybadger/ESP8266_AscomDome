#ifndef ALPACA_REQUEST_VALIDATION_H
#define ALPACA_REQUEST_VALIDATION_H

#include <stddef.h>
#include <stdint.h>

//HTTP arguments are decimal text, not JSON numeric values. Assign the output
//only after the whole input has been checked, without signed conversions.
inline bool parseAlpacaUint32(const char *text, size_t length, uint32_t &out)
{
  out = 0;
  if (text == nullptr || length == 0)
    return false;

  uint32_t value = 0;
  for (size_t i = 0; i < length; ++i)
  {
    if (text[i] < '0' || text[i] > '9')
      return false;
    const uint32_t digit = static_cast<uint32_t>(text[i] - '0');
    if (value > (UINT32_MAX - digit) / 10)
      return false;
    value = value * 10 + digit;
  }
  out = value;
  return true;
}

#endif
