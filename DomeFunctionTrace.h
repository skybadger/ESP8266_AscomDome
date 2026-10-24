#ifndef DOME_FUNCTION_TRACE_H
#define DOME_FUNCTION_TRACE_H

// Use the existing verbose debug sink. Host adapters and disabled debug builds
// compile tracing away, including evaluation of diagnostic expressions.
#if defined(debugV)
template <typename State>
class DomeFunctionTrace
{
  const char *name;
  State state;

public:
  /**
   * @brief Trace function entry and retain a state reporter for exit.
   * @details Store a stack-only callback; invoke it while function arguments and
   * object members are alive. The callback must not reference body-local objects.
   * @param[in] label (const char *) Function name with static storage duration.
   * @param[in] snapshot (State) Copyable callback reporting parameters and state.
   * @return No value (constructor); emits entry diagnostics.
   */
  DomeFunctionTrace(const char *label, State snapshot)
      : name(label), state(snapshot)
  {
    debugV("ENTER %s", name);
    state();
  }

  /**
   * @brief Trace the final function state on every exit path.
   * @details Run after body-local objects are destroyed, including early returns.
   * @par Arguments
   * None.
   * @return No value (destructor); emits exit diagnostics.
   */
  ~DomeFunctionTrace()
  {
    debugV("EXIT %s", name);
    state();
  }
};

#define DOME_FUNCTION_TRACE(...)     \
  auto domeTraceState = __VA_ARGS__; \
  DomeFunctionTrace<decltype(domeTraceState)> domeFunctionTrace(__func__, domeTraceState)
#else
#define DOME_FUNCTION_TRACE(...) \
  do                             \
  {                              \
  } while (0)
#endif

#endif
