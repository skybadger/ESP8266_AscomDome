#include "../DomeFunctionTrace.h"
//The runner extracts the production dome functions; only hardware is stubbed.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdio>
#include "../../libraries/LinkedList/LinkedList.h"
#define DOME_HEAP_SCOPE(...)
#define debugI(...)
#define debugD(...)
#define debugV(...)
#define debugW(...)
#define debugE(...)
#define F(x) x
using std::abs;
class String : public std::string
{
public:
  using std::string::string;
  /**
   * @brief Implement the equals ignore case test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the equals ignore case test adapter using in-memory state or assertions instead of physical hardware.
   * @param[in] s (const char *) Text compared by the adapter.
   * @return (bool) True on a successful check or operation; false otherwise.
   */
  bool equalsIgnoreCase(const char *s) const
  {
    return _stricmp(c_str(), s) == 0;
  }
};
enum domeCmd
{
  CMD_DOME_ABORT,
  CMD_DOME_SLEW,
  CMD_DOME_HOME,
  CMD_DOME_PARK,
  CMD_DOMEVAR_SET
};
enum shutterCmd
{
  CMD_SHUTTER_ABORT
};
enum domeState
{
  DOME_IDLE,
  DOME_SLEWING
};
enum motorSpeed
{
  MOTOR_SPEED_OFF,
  MOTOR_SPEED_SLOW_SLEW,
  MOTOR_SPEED_FAST_SLEW
};
enum motorDirection
{
  MOTOR_DIRN_CW,
  MOTOR_DIRN_CCW
};
struct cmdItem_t
{
  char *cmdName;
  int cmd, value;
  uint32_t clientId, transId;
};
LinkedList<cmdItem_t *> queue, shutterQueue;
auto *domeCmdList = &queue;
auto *shutterCmdList = &shutterQueue;
float bearing = 44, targetAzimuth = 180, currentAzimuth = 44, azimuthSyncOffset = 0;
float acceptableAzimuthError = 0.5F;
int homePosition = 0, parkPosition = 0, slowAzimuthRange = 10;
bool slewing = false, lcdPresent = false, motorPresent = true;
domeState domeStatus = DOME_SLEWING;
struct Motor
{ /**
 * @brief Implement the set speed direction test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the set speed direction test adapter using in-memory state or assertions instead of physical hardware.
 * @par Unnamed argument
 * (int) Reserved adapter argument; intentionally unused.
 * @par Unnamed argument
 * (int) Reserved adapter argument; intentionally unused.
 * @return (void) Observable effects are described above.
 */
  void setSpeedDirection(int, int)
  {
  } /**
 * @brief Implement the get speed direction test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the get speed direction test adapter using in-memory state or assertions instead of physical hardware.
 * @par Arguments
 * None.
 * @return (void) Observable effects are described above.
 */
  void getSpeedDirection()
  {
  }
} myMotor;
struct LCD
{ /**
 * @brief Implement the write lcd test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the write lcd test adapter using in-memory state or assertions instead of physical hardware.
 * @par Unnamed argument
 * (int) Reserved adapter argument; intentionally unused.
 * @par Unnamed argument
 * (int) Reserved adapter argument; intentionally unused.
 * @par Unnamed argument
 * (const char *) Reserved adapter argument; intentionally unused.
 * @return (void) Observable effects are described above.
 */
  void writeLCD(int, int, const char *)
  {
  }
} myLCD;
/**
 * @brief Implement the save to eeprom test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the save to eeprom test adapter using in-memory state or assertions instead of physical hardware.
 * @par Arguments
 * None.
 * @return (void) Observable effects are described above.
 */
void saveToEeprom()
{
}
/// @see onDomeSlew for the documented definition.
void onDomeSlew();
/// @see onDomeAbort for the documented definition.
void onDomeAbort();
static int failAfter = -1;
/**
 * @brief Implement the test calloc test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the test calloc test adapter using in-memory state or assertions instead of physical hardware.
 * @param[in] count (size_t) Number of allocations requested.
 * @param[in] size (size_t) Size of each allocation in bytes.
 * @return (void *) Caller-owned allocation, or null when failure is injected; release with free.
 */
void *testCalloc(size_t count, size_t size)
{
  if (failAfter == 0)
    return nullptr;
  if (failAfter > 0)
    --failAfter;
  return std::calloc(count, size);
}
#define calloc testCalloc
#include "../build/dome-recovery-under-test.h"
#undef calloc

/**
 * @brief Implement the start recovery test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the start recovery test adapter using in-memory state or assertions instead of physical hardware.
 * @par Arguments
 * None.
 * @return (void) Observable effects are described above.
 */
void startRecovery()
{
  targetAzimuth = bearing + 90;
  domeStatus = DOME_SLEWING;
  for (int i = 0; i < 10 && !domeLockRecoveryInProgress; ++i)
    onDomeSlew();
  assert(domeLockRecoveryInProgress && queue.size() == 3);
}
/**
 * @brief Exercise the firmware behavior with deterministic inputs and assert expected results.
 * @details Exercise the firmware behavior with deterministic inputs and assert expected results; return zero when all checks pass.
 * @par Arguments
 * None.
 * @return (int) Zero when all assertions pass; assertion failure terminates the test.
 */
int main()
{
  startRecovery();
  onDomeIdle();
  for (int i = 0; i < 1000; ++i)
    onDomeSlew();
  assert(queue.size() == 2 && domeLockRecoveryInProgress);
  bearing = targetAzimuth;
  onDomeSlew();
  assert(domeStatus == DOME_IDLE && domeLockRecoveryInProgress);
  onDomeIdle();
  bearing = targetAzimuth;
  onDomeSlew();
  assert(queue.size() == 1 && domeLockRecoveryInProgress);
  //Final recovery move already at target must complete on its first call.
  bearing = (float)queue.get(0)->value;
  onDomeIdle();
  assert(queue.size() == 0 && !domeLockRecoveryInProgress && domeStatus == DOME_IDLE);
  startRecovery();
  auto *unrelated = addDomeCmd(1, 2, "", CMD_DOME_SLEW, 270);
  onDomeIdle();
  onDomeAbort();
  assert(!domeLockRecoveryInProgress && !slewing);
  assert(queue.size() == 1 && queue.get(0) == unrelated);
  freeCmd(queue.shift());
  //Failure at each command/name allocation must remove partial recovery queues.
  for (int failure = 0; failure < 6; ++failure)
  {
    failAfter = failure;
    targetAzimuth = bearing + 90;
    domeStatus = DOME_SLEWING;
    domeLockDetectedCount = domeLockDetectedEntryCount + 1;
    onDomeSlew();
    assert(!domeLockRecoveryInProgress && queue.size() == 0 && !slewing);
    failAfter = -1;
  }
  startRecovery();
  onDomeAbort();
  assert(queue.size() == 0 && !domeLockRecoveryInProgress);
  puts("PASS: repeated stalls, all recovery legs, immediate completion, abort, restart and allocation failures");
}
