#include "../DomeFunctionTrace.h"
//Host-side HTTP/queue doubles exercise the actual UI control handler without hardware.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <map>
#include <string>
#include <vector>

#define F(value) value
#ifndef TEST_DISABLED
#define _ENABLE_DOME
#define _ENABLE_SHUTTER
#endif
#define SHUTTER_MIN_ALTITUDE 0
#define SHUTTER_MAX_ALTITUDE 110

class String : public std::string
{
public:
  /**
   * @brief Implement the string test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the string test adapter using in-memory state or assertions instead of physical hardware.
   * @par Arguments
   * None.
   * @return No value (constructor).
   */
  String() = default;
  using std::string::string;
  /**
   * @brief Implement the string test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the string test adapter using in-memory state or assertions instead of physical hardware.
   * @param[in] value (const std::string &) Operation value or field text.
   * @return No value (constructor).
   */
  String(const std::string &value)
      : std::string(value)
  {
  }
  /**
   * @brief Implement the string test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the string test adapter using in-memory state or assertions instead of physical hardware.
   * @param[in] value (int) Operation value or field text.
   * @return No value (constructor).
   */
  String(int value)
      : std::string(std::to_string(value))
  {
  }
};
enum
{
  HTTP_GET,
  HTTP_POST,
  HTTP_PUT
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
  CMD_SHUTTER_ABORT,
  CMD_SHUTTER_OPEN = 4,
  CMD_SHUTTER_CLOSE,
  CMD_SHUTTERVAR_SET
};
enum
{
  DOME_IDLE,
  DOME_SLEWING,
  DOME_ABORT,
  DOME_HALTED
};
enum
{
  SHUTTER_OPEN,
  SHUTTER_CLOSED,
  SHUTTER_OPENING,
  SHUTTER_CLOSING,
  SHUTTER_ERROR,
  SHUTTER_ABORTING,
  SHUTTER_ABORTED,
  SHUTTER_HALTED
};
struct cmdItem_t
{
  int cmd;
  int value;
  String name;
};
struct Queue
{
  std::vector<cmdItem_t> items;
  /**
   * @brief Implement the size test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the size test adapter using in-memory state or assertions instead of physical hardware.
   * @par Arguments
   * None.
   * @return (int) Number of items in the simulated command queue.
   */
  int size()
  {
    return static_cast<int>(items.size());
  }
  /**
   * @brief Implement the get test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the get test adapter using in-memory state or assertions instead of physical hardware.
   * @param[in] index (int) Zero-based queue index.
   * @return (cmdItem_t *) Borrowed pointer to the indexed queue item; invalidated by queue mutation; never free it.
   */
  cmdItem_t *get(int index)
  {
    return &items.at(index);
  }
};
Queue domeQueue, shutterQueue;
Queue *domeCmdList = &domeQueue, *shutterCmdList = &shutterQueue;
int domeStatus = DOME_IDLE, shutterStatus = SHUTTER_CLOSED;
bool motorPresent = true, failAllocation = false;
float targetAzimuth = 0;
int homePosition = 180, parkPosition = 356;
const char *shutterHostname = "shutter";
int statusCode = 0;
struct Server
{
  int verb = HTTP_POST;
  std::map<String, String> fields;
  /**
   * @brief Implement the method test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the method test adapter using in-memory state or assertions instead of physical hardware.
   * @par Arguments
   * None.
   * @return (int) Cached or computed value described above.
   */
  int method()
  {
    return verb;
  }
  /**
   * @brief Implement the args test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the args test adapter using in-memory state or assertions instead of physical hardware.
   * @par Arguments
   * None.
   * @return (int) Cached or computed value described above.
   */
  int args()
  {
    return static_cast<int>(fields.size());
  }
  /**
   * @brief Implement the has arg test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the has arg test adapter using in-memory state or assertions instead of physical hardware.
   * @param[in] key (const char *) Configuration field key.
   * @return (bool) True on a successful check or operation; false otherwise.
   */
  bool hasArg(const char *key)
  {
    return fields.count(key) != 0;
  }
  /**
   * @brief Implement the arg test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the arg test adapter using in-memory state or assertions instead of physical hardware.
   * @param[in] key (const char *) Configuration field key.
   * @return (String) Generated field or markup text.
   */
  String arg(const char *key)
  {
    return hasArg(key) ? fields.at(key) : String();
  }
  /**
   * @brief Implement the send header test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the send header test adapter using in-memory state or assertions instead of physical hardware.
   * @par Unnamed argument
   * (const char *) Reserved adapter argument; intentionally unused.
   * @par Unnamed argument
   * (const char *) Reserved adapter argument; intentionally unused.
   * @return (void) Observable effects are described above.
   */
  void sendHeader(const char *, const char *)
  {
  }
  /**
   * @brief Implement the send test adapter using in-memory state or assertions instead of physical hardware.
   * @details Implement the send test adapter using in-memory state or assertions instead of physical hardware.
   * @param[in] status (int) HTTP response status.
   * @par Unnamed argument
   * (const char *) Reserved adapter argument; intentionally unused.
   * @par Unnamed argument
   * (const char *) Reserved adapter argument; intentionally unused.
   * @return (void) Observable effects are described above.
   */
  void send(int status, const char *, const char *)
  {
    statusCode = status;
  }
} server;
/**
 * @brief Implement the send setup page test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the send setup page test adapter using in-memory state or assertions instead of physical hardware.
 * @par Unnamed argument
 * (bool) Reserved adapter argument; intentionally unused.
 * @param[in] status (int) HTTP response status.
 * @par Unnamed argument
 * (const String &) Reserved adapter argument; intentionally unused.
 * @return (void) Observable effects are described above.
 */
void sendSetupPage(bool, int status, const String &)
{
  statusCode = status;
}
/**
 * @brief Implement the normalise int test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the normalise int test adapter using in-memory state or assertions instead of physical hardware.
 * @param[in,out] value (int &) Operation value or field text.
 * @param[in] radix (int) Positive modulus defining the wrap interval.
 * @return (int) Normalized value, also stored in the referenced argument.
 */
int normaliseInt(int &value, int radix)
{
  value = (value % radix + radix) % radix;
  return value;
}
/**
 * @brief Implement the add dome cmd test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the add dome cmd test adapter using in-memory state or assertions instead of physical hardware.
 * @par Unnamed argument
 * (uint32_t) Reserved adapter argument; intentionally unused.
 * @par Unnamed argument
 * (uint32_t) Reserved adapter argument; intentionally unused.
 * @param[in] name (String) Test command label.
 * @param[in] command (domeCmd) Command operation used by the test adapter.
 * @param[in] value (int) Operation value or field text.
 * @return (cmdItem_t *) Queued command owned by the queue; null on allocation failure. Release through freeCmd after removing it.
 */
cmdItem_t *addDomeCmd(uint32_t, uint32_t, String name, domeCmd command, int value)
{
  if (failAllocation)
    return nullptr;
  domeQueue.items.push_back({command, value, name});
  return &domeQueue.items.back();
}
/**
 * @brief Implement the add shutter cmd test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the add shutter cmd test adapter using in-memory state or assertions instead of physical hardware.
 * @par Unnamed argument
 * (uint32_t) Reserved adapter argument; intentionally unused.
 * @par Unnamed argument
 * (uint32_t) Reserved adapter argument; intentionally unused.
 * @param[in] name (String) Test command label.
 * @param[in] command (shutterCmd) Command operation used by the test adapter.
 * @param[in] value (int) Operation value or field text.
 * @return (cmdItem_t *) Queued command owned by the queue; null on allocation failure. Release through freeCmd after removing it.
 */
cmdItem_t *addShutterCmd(uint32_t, uint32_t, String name, shutterCmd command, int value)
{
  if (failAllocation)
    return nullptr;
  shutterQueue.items.push_back({command, value, name});
  return &shutterQueue.items.back();
}
#include "../ASCOM_DomeControl.h"

/**
 * @brief Implement the request test adapter using in-memory state or assertions instead of physical hardware.
 * @details Implement the request test adapter using in-memory state or assertions instead of physical hardware.
 * @param[in] action (const char *) Simulated HTTP movement action.
 * @param[in] value (const char *) Operation value or field text.
 * @return (void) Observable effects are described above.
 */
void request(const char *action, const char *value = nullptr)
{
  server.fields = {{"action", action}};
  if (value)
    server.fields["value"] = value;
  statusCode = 0;
  handleDomeUiControl();
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
  server.verb = HTTP_GET;
  request("slew", "180");
  assert(statusCode == 405 && domeQueue.size() == 0);
  server.verb = HTTP_POST;
  for (const char *bad : {"", "-1", "361", "1.5", "99junk", "4294967296"})
  {
    request("slew", bad);
    assert(statusCode == 400 && domeQueue.size() == 0);
  }
  request("altitude", "111");
  assert(statusCode == 400 && shutterQueue.size() == 0);
  request("unknown");
  assert(statusCode == 400);
  request("slew");
  assert(statusCode == 400);
#ifdef TEST_DISABLED
  request("slew", "180");
  assert(statusCode == 503 && domeQueue.size() == 0);
  request("open");
  assert(statusCode == 503 && shutterQueue.size() == 0);
  puts("PASS: disabled control loops reject movement; invalid requests do not queue commands");
#else
  request("slew", "360");
  assert(statusCode == 303 && domeQueue.items.back().value == 0);
  domeQueue.items.clear();
  targetAzimuth = 358;
  request("east");
  assert(statusCode == 303 && domeQueue.items.back().value == 3 && targetAzimuth == 358);
  request("east");
  assert(domeQueue.items.back().value == 8);
  request("west");
  assert(domeQueue.items.back().value == 3);
  domeQueue.items.clear();
  targetAzimuth = 2;
  request("west");
  assert(domeQueue.items.back().value == 357);
  domeQueue.items.clear();
  domeStatus = DOME_SLEWING;
  targetAzimuth = 120;
  request("slew", "240");
  assert(statusCode == 303 && domeQueue.items.back().value == 240 && targetAzimuth == 120);
  domeQueue.items.clear();
  domeStatus = DOME_HALTED;
  request("east");
  assert(statusCode == 409 && domeQueue.size() == 0);
  domeStatus = DOME_IDLE;
  motorPresent = false;
  request("slew", "180");
  assert(statusCode == 503 && domeQueue.size() == 0);
  motorPresent = true;
  failAllocation = true;
  request("slew", "180");
  assert(statusCode == 503);
  failAllocation = false;
  for (int i = 0; i < domeUiQueueLimit; ++i)
    request("east");
  request("east");
  assert(statusCode == 409 && domeQueue.size() == domeUiQueueLimit);
  request("open");
  assert(statusCode == 303 && shutterQueue.items.back().cmd == CMD_SHUTTER_OPEN);
  request("close");
  assert(statusCode == 303 && shutterQueue.items.back().cmd == CMD_SHUTTER_CLOSE);
  for (const char *angle : {"0", "45", "110"})
  {
    request("altitude", angle);
    const auto &command = shutterQueue.items.back();
    assert(statusCode == 303 && command.cmd == CMD_SHUTTERVAR_SET && command.name == "altitude");
    assert(command.value == std::stoi(angle));
  }
  puts("PASS: slew, wrapping/accumulating jogs, queued target isolation, shutter commands, altitude endpoints and failure paths");
#endif
  return 0;
}
