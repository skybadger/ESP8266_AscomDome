#ifndef DOME_HEAP_TRACE_H
#define DOME_HEAP_TRACE_H

#if DOME_HEAP_TRACE
#include <Arduino.h>
#include <Esp.h>

/// @see domeHeapQueueSize for the documented definition.
int domeHeapQueueSize();
/// @see shutterHeapQueueSize for the documented definition.
int shutterHeapQueueSize();

// Stack-only guard: declare before local objects so EXIT follows their destruction.
// Direct serial output avoids the RemoteDebug allocation path.
class DomeHeapScope
{
  const char *name;
  bool enabled;
  uint32_t id = 0;
  uint32_t entryHeap = 0;
  int entryQueue = 0;
  unsigned depth = 0;

  /**
   * @brief Return the static heap-trace event sequence counter by reference.
   * @details Return the static heap-trace event sequence counter by reference.
   * @par Arguments
   * None.
   * @return (uint32_t &) Reference to the shared monotonic event counter.
   */
  static uint32_t &sequence()
  {
    static uint32_t value = 0;
    return value;
  }
  /**
   * @brief Return the static heap-trace nesting depth by reference.
   * @details Return the static heap-trace nesting depth by reference.
   * @par Arguments
   * None.
   * @return (unsigned &) Reference to the shared nesting counter.
   */
  static unsigned &nesting()
  {
    static unsigned value = 0;
    return value;
  }

  /**
   * @brief Write timestamp, scope, free heap, fragmentation and command queue deltas directly to serial without allocating strings.
   * @details Write timestamp, scope, free heap, fragmentation and command queue deltas directly to serial without allocating strings.
   * @param[in] phase (const char *) Entry or exit marker.
   * @param[in] heap (uint32_t) Free heap size in bytes.
   * @param[in] queue (int) Current dome queue size.
   * @param[in] delta (int32_t) Signed heap change from entry.
   * @return (void) Observable effects are described above.
   */
  void report(const char *phase, uint32_t heap, int queue, int32_t delta)
  {
    Serial.printf_P(PSTR("[HEAP] ms=%lu id=%lu depth=%u %s %s free=%lu delta=%ld max=%lu frag=%u dq=%d dqDelta=%d sq=%d\n"),
                    (unsigned long)millis(), (unsigned long)id, depth, phase, name,
                    (unsigned long)heap, (long)delta,
                    (unsigned long)ESP.getMaxFreeBlockSize(), (unsigned)ESP.getHeapFragmentation(),
                    queue, queue - entryQueue, shutterHeapQueueSize());
  }

public:
  /**
   * @brief Capture initial heap and queue sizes and report entry when active.
   * @details Capture initial heap and queue sizes and report entry when active; disabled scopes do not sample state.
   * @param[in] label (const char *) Borrowed static scope name.
   * @param[in] active (bool) Whether to emit heap diagnostics for this scope.
   * @return No value (constructor).
   */
  explicit DomeHeapScope(const char *label, bool active = true)
      : name(label), enabled(active)
  {
    if (!enabled)
      return;
    id = ++sequence();
    depth = nesting()++;
    entryHeap = ESP.getFreeHeap();
    entryQueue = domeHeapQueueSize();
    report("ENTER", entryHeap, entryQueue, 0);
  }
  /**
   * @brief Report final heap and queue deltas and decrement nesting depth when this scope is active.
   * @details Report final heap and queue deltas and decrement nesting depth when this scope is active.
   * @par Arguments
   * None.
   * @return No value (destructor).
   */
  ~DomeHeapScope()
  {
    if (!enabled)
      return;
    const uint32_t heap = ESP.getFreeHeap();
    report("EXIT", heap, domeHeapQueueSize(), (int32_t)heap - (int32_t)entryHeap);
    --nesting();
  }
  /**
   * @brief Capture initial heap and queue sizes and report entry when active.
   * @details Capture initial heap and queue sizes and report entry when active; disabled scopes do not sample state.
   * @par Unnamed argument
   * (const DomeHeapScope &) Reserved adapter argument; intentionally unused.
   * @return No value (constructor).
   */
  DomeHeapScope(const DomeHeapScope &) = delete;
  /**
   * @brief Disallow copying the heap scope so a single owner reports its lifetime.
   * @details Disallow copying the heap scope so a single owner reports its lifetime.
   * @par Unnamed argument
   * (const DomeHeapScope &) Reserved adapter argument; intentionally unused.
   * @return (DomeHeapScope &) Cached or computed value described above.
   */
  DomeHeapScope &operator=(const DomeHeapScope &) = delete;
};
#define DOME_HEAP_SCOPE(label) DomeHeapScope domeHeapScope(label)
#define DOME_HEAP_SCOPE_IF(label, active) DomeHeapScope domeHeapScope(label, active)
#else
#define DOME_HEAP_SCOPE(label) \
  do                           \
  {                            \
  } while (0)
#define DOME_HEAP_SCOPE_IF(label, active) \
  do                                      \
  {                                       \
  } while (0)
#endif
#endif
