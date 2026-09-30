#ifndef DOME_BEARING_SYNC_H
#define DOME_BEARING_SYNC_H

#include <math.h>

//Raw sensor coordinates remain unchanged. Store the shortest signed correction.
inline bool calculateDomeBearingSync(float raw, float known, float &offset, float &azimuth)
{
  if (!isfinite(raw) || !isfinite(known) || raw < 0 || raw > 360 || known < 0 || known > 360)
    return false;
  const float corrected = known == 360.0F ? 0.0F : known;
  offset = fmodf(corrected - raw + 540.0F, 360.0F) - 180.0F;
  azimuth = corrected;
  return true;
}

#endif
