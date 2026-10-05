#ifndef DOME_HEAP_TRACE_H
#define DOME_HEAP_TRACE_H

#if DOME_HEAP_TRACE
#include <Arduino.h>
#include <Esp.h>

int domeHeapQueueSize();
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

  static uint32_t &sequence() { static uint32_t value = 0; return value; }
  static unsigned &nesting() { static unsigned value = 0; return value; }

  void report(const char *phase, uint32_t heap, int queue, int32_t delta)
  {
    Serial.printf_P(PSTR("[HEAP] ms=%lu id=%lu depth=%u %s %s free=%lu delta=%ld max=%lu frag=%u dq=%d dqDelta=%d sq=%d\n"),
                    (unsigned long)millis(), (unsigned long)id, depth, phase, name,
                    (unsigned long)heap, (long)delta,
                    (unsigned long)ESP.getMaxFreeBlockSize(), (unsigned)ESP.getHeapFragmentation(),
                    queue, queue - entryQueue, shutterHeapQueueSize());
  }

public:
  explicit DomeHeapScope(const char *label, bool active = true) : name(label), enabled(active)
  {
    if (!enabled) return;
    id = ++sequence();
    depth = nesting()++;
    entryHeap = ESP.getFreeHeap();
    entryQueue = domeHeapQueueSize();
    report("ENTER", entryHeap, entryQueue, 0);
  }
  ~DomeHeapScope()
  {
    if (!enabled) return;
    const uint32_t heap = ESP.getFreeHeap();
    report("EXIT", heap, domeHeapQueueSize(), (int32_t)heap - (int32_t)entryHeap);
    --nesting();
  }
  DomeHeapScope(const DomeHeapScope &) = delete;
  DomeHeapScope &operator=(const DomeHeapScope &) = delete;
};
#define DOME_HEAP_SCOPE(label) DomeHeapScope domeHeapScope(label)
#define DOME_HEAP_SCOPE_IF(label, active) DomeHeapScope domeHeapScope(label, active)
#else
#define DOME_HEAP_SCOPE(label) do {} while (0)
#define DOME_HEAP_SCOPE_IF(label, active) do {} while (0)
#endif
#endif
