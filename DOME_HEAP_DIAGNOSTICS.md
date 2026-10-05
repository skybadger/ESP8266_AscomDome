# Dome loop heap investigation

## Evidence from the monitor

Reviewed `logs/device-monitor-261005-103319.log` (latest monitor log on 5 October 2026). The file was still being written, so these counts describe the snapshot read during investigation:

- 142 `DOMELOCK detected` events.
- 428 `Cmd added: 1` messages, 143 commands popped, and 143 `Cmd freed` messages.
- At lines 403–445, free heap is 36144 before recovery, 36000 after three commands are added, and 36048 after one command is consumed: -144 followed by +48 bytes.
- Bearing remains 44.010227 in the sampled slew sequences while targets change.

The source currently enables `_ENABLE_DOME`, with `_ENABLE_BEARING` and `_ENABLE_SHUTTER` commented out. An unchanged bearing satisfies the stall condition repeatedly. `onDomeSlew()` enqueues three recovery moves and returns to idle; `onDomeIdle()` consumes one move and resets the stall counter. If that move also stalls, another three are added. The backlog therefore grows by two commands per repeated recovery cycle (approximately 96 bytes with the observed allocations). Recovery commands themselves can initiate further recovery. `addDomeCmd()` has no queue bound.

This is strong evidence for retained command-queue growth, rather than a missing `free()` in every normal slew call. It does not exclude other losses. These observations describe the code before the recovery guard below was added.

## Recovery guard

`domeLockRecoveryInProgress` is set before enqueuing the three recovery moves and prevents nested recovery sequences. It remains set through the intermediate moves and clears only when the final return move reaches its target. A recovery move that remains stalled does not enqueue more moves; the flag remains set until completion or abort.

Abort removes and frees the remaining recovery commands while retaining unrelated queued commands, then resets the recovery state. Failure to allocate any recovery command cancels the partial sequence and stops the motor. Queued recovery pointers are cleared on dequeue to avoid retaining freed command pointers. The state changes to SLEWING before invoking the slew handler, so immediate target completion is preserved.

`tests/test-dome-recovery.ps1` extracts the production dome functions and runs them with hardware stubs using the existing MSVC host-test toolchain. It covers 1,000 repeated stalled calls without queue growth, intermediate and final completion, immediate final completion, abort preserving unrelated commands, recovery restart, and failure at each command/name allocation. This regression and the `esp12e` build passed; hardware validation is pending.

## Call map

```text
loop
├─ fineTimerFlag / _ENABLE_BEARING
│  ├─ getBearing -> restQuery -> WiFiClient / HTTPClient -> JSON parsing
│  └─ getAzimuth -> normaliseFloat
├─ coarseTimerFlag
│  ├─ loop.domeDispatch / _ENABLE_DOME
│  │  ├─ DOME_IDLE -> onDomeIdle
│  │  │  ├─ domeCmdList.shift (deletes list node)
│  │  │  ├─ HOME / PARK / SLEW -> onDomeSlew
│  │  │  ├─ ABORT -> onDomeAbort
│  │  │  ├─ VAR_SET -> saveToEeprom -> writeSetupExtension / EEPROM.commit
│  │  │  └─ freeCmd (frees command name and command)
│  │  ├─ DOME_SLEWING -> onDomeSlew
│  │  │  ├─ getAzimuth -> normaliseFloat
│  │  │  ├─ motor.setSpeedDirection -> Wire transmission
│  │  │  ├─ LCD.writeLCD, if present -> calloc / Wire / free
│  │  │  ├─ stalled: normaliseInt -> addDomeCmd × 3
│  │  │  │  └─ command calloc + name calloc + LinkedList.add node allocation
│  │  │  ├─ stalled: motor.setSpeedDirection(OFF), state becomes IDLE
│  │  │  └─ motor.getSpeedDirection -> Wire transmission / request / read
│  │  └─ DOME_ABORT -> onDomeAbort -> motor stop / LCD
│  ├─ _ENABLE_SHUTTER -> getShutterStatus -> restQuery; shutter dispatch
│  ├─ loop.clockLCD -> getTimeAsString / substring / LCD.writeLCD
│  └─ updateStatusPollingPeriod -> timer disarm/rearm when needed
├─ loop.mqtt -> reconnectNB / client.loop / status publishing
├─ server.handleClient -> registered REST/UI handlers -> command enqueues as requested
├─ manageConnectionState -> optional park/close command enqueues
├─ handleManagement -> Alpaca discovery
├─ Debug.handle, if enabled
└─ delay(20)
```

Guards cover the dome functions above, motor read/write, LCD write, EEPROM save, bearing/shutter polling and REST queries. Parent guards bracket dome dispatch and the clock LCD block. The outer loop, MQTT, HTTP server, connection management, discovery and RemoteDebug guards sample loops whose coarse timer flag was set at entry, to avoid printing every 20 ms. Dome function guards run on every invocation, including calls from web handlers. The map includes library calls; individual Wire, EEPROM, networking and shared-library internals are not separately instrumented.

## Reading the trace

`DOME_HEAP_TRACE` in `ESP8266_AscomDome.h` enables tracing by default. Set `-D DOME_HEAP_TRACE=0` in PlatformIO build flags (or change its default) to compile it out. Existing feature switches are preserved. Build/upload using the normal workflow, then capture serial output at 115200 baud and search for `[HEAP]`.

Each ENTER/EXIT pair shares `id` and `depth`. `ms` is uptime. `free` is free heap; EXIT `delta` is exit minus entry bytes. `max` is the largest available block and `frag` the fragmentation percentage. `dq`/`sq` are dome/shutter queue lengths; `dqDelta` is the dome queue change over the scope.

- Repeated `onDomeSlew` EXIT with `delta=-144 dqDelta=3`, followed by `onDomeIdle` EXIT with `delta=48 dqDelta=-1`, would confirm the backlog mechanism (exact bytes may vary).
- A repeating negative delta with unchanged queues points to the deepest enclosed scope showing the loss. Do not sum nested deltas: parent deltas already include children.
- `freeCmd` only measures the payload release; the list node was freed by `shift()` before it was called. `addDomeCmd` includes both payload and node allocations.
- Largest-block decline with stable total free heap suggests fragmentation rather than retained byte growth.
- Local objects are destroyed before a guard reports EXIT, including on early returns. By-value function arguments and caller temporaries can outlive that report; use the enclosing call scope to account for them.
- The guard itself uses stack fields and fixed-format serial output, without `String` or explicit heap allocations. Serial/library logging and background networking can still affect measurements and timing. Compare repeated cycles after startup; a single negative delta is not proof of a leak.

The legacy `checkRam()`/`debugMem()` helpers, RAM baseline variables and `_TEST_RAM_`, `_MEMLEAK_CHECK`, `_MEMLEAK_CHECK_DEBUG` defines have been removed. Loop tracing now uses only `DomeHeapScope`, controlled by `DOME_HEAP_TRACE`. `loop.bearing`, `loop.domeDispatch` and `loop.shutterDispatch` bracket their respective enabled timer sections. The outer `loop.coarseTick` scope retains coarse-tick sampling and reports after local strings have been destroyed.
