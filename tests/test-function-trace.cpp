#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

static std::vector<std::string> events;

/**
 * @brief Record scoped trace messages for assertions.
 * @details Format diagnostics into a bounded buffer and retain their order.
 * @param[in] format (const char *) Printf-compatible format string.
 * @param[in] arguments (Args...) Typed arguments used by the format string.
 * @return (void) Appends one message to the test event list.
 */
template <typename... Args>
void recordTrace(const char *format, Args... arguments)
{
  char buffer[128];
  std::snprintf(buffer, sizeof(buffer), format, arguments...);
  events.emplace_back(buffer);
}

#ifndef TEST_TRACE_DISABLED
#define debugV(...) recordTrace(__VA_ARGS__)
#endif
#include "../DomeFunctionTrace.h"

/**
 * @brief Exercise a successful or early function return under tracing.
 * @details Mutate the referenced state after entry and return through one of
 * two branches so the test can verify exactly one exit and its final snapshot.
 * @param[in,out] state (int &) State inspected at entry and exit.
 * @param[in] early (bool) Whether to use the early return branch.
 * @return (int) 7 for early return; otherwise 9.
 */
int exerciseTrace(int &state, bool early)
{
  DOME_FUNCTION_TRACE([&]()
                      {
                        debugV("state=%d", state);
                      });
  state = 42;
  if (early)
    return 7;
  return 9;
}

/**
 * @brief Verify scoped trace lifetime and disabled tracing behavior.
 * @details Assert entry and exit ordering, final state on both return branches,
 * and absence of diagnostic side effects when the debug sink is unavailable.
 * @par Arguments
 * None.
 * @return (int) Zero when all assertions pass; assertion failure terminates the test.
 */
int main()
{
  for (bool early : {false, true})
  {
    events.clear();
    int state = 3;
    assert(exerciseTrace(state, early) == (early ? 7 : 9));
    assert(state == 42);
#ifndef TEST_TRACE_DISABLED
    assert((events == std::vector<std::string>{"ENTER exerciseTrace", "state=3", "EXIT exerciseTrace", "state=42"}));
#else
    assert(events.empty());
#endif
  }
  std::puts("PASS: function trace return paths and state snapshots");
  return 0;
}
