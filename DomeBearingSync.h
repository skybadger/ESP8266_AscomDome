#include "DomeFunctionTrace.h"
#ifndef DOME_BEARING_SYNC_H
#define DOME_BEARING_SYNC_H

#include <math.h>

//Raw sensor coordinates remain unchanged. Store the shortest signed correction.
/**
 * @brief Reject non-finite or out-of-range bearings.
 * @details Reject non-finite or out-of-range bearings; calculate the shortest signed correction and normalized known azimuth.
 * @param[in] raw (float) Raw bearing in degrees.
 * @param[in] known (float) Known physical azimuth in degrees.
 * @param[in,out] offset (float &) Calculated signed bearing correction in degrees.
 * @param[in,out] azimuth (float &) Calculated normalized azimuth in degrees.
 * @return (bool) True when both outputs were calculated; false for invalid bearings, leaving outputs unchanged.
 */
inline bool calculateDomeBearingSync(float raw, float known, float &offset, float &azimuth)
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("raw=%g", (double)raw);
                        debugV("known=%g", (double)known);
                        debugV("offset=%g", (double)offset);
                        debugV("azimuth=%g", (double)azimuth);
                      });
  if (!isfinite(raw) || !isfinite(known) || raw < 0 || raw > 360 || known < 0 || known > 360)
    return false;
  const float corrected = known == 360.0F ? 0.0F : known;
  offset = fmodf(corrected - raw + 540.0F, 360.0F) - 180.0F;
  azimuth = corrected;
  return true;
}

#endif
