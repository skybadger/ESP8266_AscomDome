#include "../DomeBearingSync.h"
#include <assert.h>
#include <limits>
#include <stdio.h>

int main()
{
  struct Case
  {
    float raw, known, expected;
  };
  const Case cases[] = {{350, 10, 20}, {10, 350, -20}, {123.25F, 125.75F, 2.5F}, {0, 360, 0}, {360, 0, 0}, {180, 0, -180}, {42, 42, 0}};
  for (const auto &test : cases)
  {
    float offset = 999, azimuth = 999;
    assert(calculateDomeBearingSync(test.raw, test.known, offset, azimuth));
    assert(fabsf(offset - test.expected) < 0.001F);
    const float expected = test.known == 360 ? 0 : test.known;
    assert(fabsf(azimuth - expected) < 0.001F);
    assert(fabsf(fmodf(test.raw + offset + 360, 360) - expected) < 0.001F);
  }
  const float invalid[] = {-1, 361, std::numeric_limits<float>::infinity(),
                           std::numeric_limits<float>::quiet_NaN()};
  for (float value : invalid)
  {
    float offset = 12, azimuth = 34;
    assert(!calculateDomeBearingSync(value, 90, offset, azimuth));
    assert(offset == 12 && azimuth == 34);
    assert(!calculateDomeBearingSync(90, value, offset, azimuth));
    assert(offset == 12 && azimuth == 34);
  }
  puts("PASS: bearing sync wraparound, fractional degrees, endpoints and invalid inputs");
}
