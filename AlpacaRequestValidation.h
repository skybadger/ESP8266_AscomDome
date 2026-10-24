#include "DomeFunctionTrace.h"
#ifndef ALPACA_REQUEST_VALIDATION_H
#define ALPACA_REQUEST_VALIDATION_H

#include <stddef.h>
#include <stdint.h>

//HTTP arguments are decimal text, not JSON numeric values. Assign the output
//only after the whole input has been checked, without signed conversions.
/**
 * @brief Parse decimal HTTP text without signed conversion.
 * @details Parse decimal HTTP text without signed conversion; reject empty input, non-digits and overflow, clearing the output on failure.
 * @param[in] text (const char *) Decimal character buffer; borrowed for this call.
 * @param[in] length (size_t) Number of input bytes.
 * @param[in,out] out (uint32_t &) Parsed unsigned value; cleared on invalid input.
 * @return (bool) True for a complete uint32 decimal value; false for empty, malformed or overflowing input.
 */
inline bool parseAlpacaUint32(const char *text, size_t length, uint32_t &out)
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("text storage=%p", (const void *)text);
                        debugV("length=%g", (double)length);
                        debugV("out storage=%p", (const void *)&out);
                      });
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
