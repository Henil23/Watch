# Watch

### macOS System Intelligence & Adaptive Network Policy Engine

**Language:** C++20/23 + Swift/SwiftUI  
**Platform:** macOS  
**Development Environment:** Xcode  
**Project Type:** Advanced systems programming / macOS utility  
**Initial Status:** Architecture & MVP Definition  
**Working Name:** Watch

---

# 1. Project Vision

Watch is a macOS system-observability and adaptive network-control utility designed to answer three questions:

1. **What is my Mac doing?**
2. **Why is it happening?**
3. **Should the system do something about it?**

Traditional monitoring applications primarily display current metrics:

- CPU usage
- memory usage
- network traffic
- disk activity
- running processes

Watch will go beyond displaying numbers.

It will maintain historical system state, correlate events across resources and processes, identify abnormal behaviour, create system "incidents", and eventually enforce user-defined network policies.

The overall model is:

```text
OBSERVE
   ↓
UNDERSTAND
   ↓
EXPLAIN
   ↓
ACT
```

Example:

```text
Watch Incident #142

13:42:08

System CPU:
22% → 91%

Network Upload:
1.1 MB/s → 19.4 MB/s

Disk Read:
32 MB/s → 341 MB/s

Primary contributors:

Dropbox
CPU               31%
Upload             18 MB/s
Disk Read          214 MB/s

Docker
CPU               27%
Disk Read          101 MB/s

Chrome
CPU               14%

Possible explanation:

Dropbox began synchronizing a large amount of data while
Docker was performing disk-intensive activity.

Duration:
6m 42s
```

Eventually Watch may provide an action:

```text
Suggested Action

Dropbox is generating significant network activity while
connected through a mobile hotspot.

[Block Until Network Changes]
[Allow]
[Create Rule]
```

---

# 2. Core Product Idea

Watch combines two traditionally separate categories of software.

### System observability

Similar concepts to:

```text
Activity Monitor
htop
top
iStat
performance monitoring tools
```

but with historical context and incident analysis.

### Network policy

Similar concepts to:

```text
application firewall
network content filtering
application network permissions
network profiles
```

but controlled by a custom policy engine.

The important difference is that these two systems communicate.

```text
SYSTEM OBSERVATION
       │
       ▼
CONTEXT ENGINE
       │
       ▼
POLICY ENGINE
       │
       ▼
NETWORK ENFORCEMENT
```

A policy can therefore eventually contain contextual conditions.

Example:

```text
IF

network = hotspot

AND

application = Steam

THEN

block network access
```

Another example:

```text
IF

network = hotspot

AND

application = Dropbox

AND

upload activity is high

THEN

notify user
```

---

# 3. Project Goals

## Primary goals

Watch should eventually be capable of:

- monitoring system-wide CPU utilisation
- monitoring memory usage
- monitoring disk activity
- discovering running processes
- maintaining process identities
- monitoring process resource consumption
- maintaining historical system metrics
- detecting abnormal resource spikes
- correlating processes with system events
- creating incident reports
- observing network flows
- associating network flows with applications where supported
- applying allow/block policies through macOS-supported networking APIs
- switching policies according to system/network context
- exposing all information through a native macOS interface

---

# 4. Non-Goals

The following are deliberately excluded from the initial project.

## No arbitrary bandwidth throttling

Watch V1 will **not promise**:

```text
Dropbox → exactly 500 KB/s
Chrome  → exactly 4 Mbps
Zoom    → guaranteed 10 Mbps
```

macOS Network Extension exposes supported filtering mechanisms, but arbitrary application bandwidth scheduling is not a capability we should assume exists.

The policy model will therefore initially use:

```text
ALLOW
BLOCK
ASK
NOTIFY
```

rather than arbitrary bandwidth limits.

---

## No kernel extension

Watch will not rely on legacy kernel extensions.

Architecture should remain in user space using supported macOS APIs and system extensions.

---

## No antivirus functionality

Watch is not intended to:

- detect malware
- scan binaries
- classify malicious applications
- replace Endpoint Detection and Response products

---

## No packet payload surveillance

Watch is primarily interested in:

```text
application
destination
connection
traffic quantity
timing
policy decision
```

not harvesting user network content.

---

## No cloud requirement

The initial product should function completely locally.

No user telemetry or cloud backend should be required.

---

# 5. Technical Feasibility

Apple provides the Network Extension framework for extending macOS networking behaviour.

Content Filter Providers can receive network flows and determine whether traffic should be allowed or blocked.

Each `NEFilterFlow` corresponds to a network connection opened by an application.

Watch will eventually use this supported mechanism for network enforcement.


Network content filtering on macOS can be deployed using a system extension. Apple documents support for macOS content-filter system extensions beginning with macOS 10.15.

System Extensions allow network-extension functionality to operate in user space rather than requiring kernel-level code.

---

# 6. Important Endpoint Security Decision

macOS also provides the Endpoint Security framework.

It can receive operating-system events such as:

- process execution
- forks
- signals
- filesystem mounts

and other security-sensitive events.

However:

```text
com.apple.developer.endpoint-security.client
```

is an entitlement that must be requested from Apple.

Therefore:

## Endpoint Security will NOT be required for MVP.

Initial process monitoring will rely on APIs accessible without making the entire project dependent on Endpoint Security approval.

Endpoint Security can later become an optional advanced backend.

Architecture must therefore define:

```cpp
class IProcessEventSource
{
public:
    virtual ~IProcessEventSource() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
};
```

Potential implementations:

```text
PollingProcessSource
EndpointSecurityProcessSource
```

This avoids coupling the engine to one API.

---

# 7. High-Level Architecture

```text
┌───────────────────────────────────────────────┐
│                 Watch.app                │
│                                               │
│                  SwiftUI UI                   │
└──────────────────────┬────────────────────────┘
                       │
                       │ IPC
                       ▼
┌───────────────────────────────────────────────┐
│                Watch Core                │
│                    C++                        │
│                                               │
│  ┌────────────┐   ┌─────────────┐             │
│  │ Collectors │ → │ Event Engine│             │
│  └────────────┘   └──────┬──────┘             │
│                           │                    │
│                           ▼                    │
│                    ┌────────────┐              │
│                    │ Incident   │              │
│                    │ Detector   │              │
│                    └─────┬──────┘              │
│                          │                     │
│         ┌────────────────┴──────────────┐      │
│         ▼                               ▼      │
│   Time-Series DB                  Policy Engine│
│                                               │
└──────────────────────┬────────────────────────┘
                       │
                       │ Rules
                       ▼
┌───────────────────────────────────────────────┐
│           Network Filter Extension            │
│                                               │
│          Apple Network Extension              │
│                                               │
│              Allow / Block                    │
└───────────────────────────────────────────────┘
```

---

# 8. Component Architecture

The backend should be divided into the following subsystems.

```text
Watch Core

├── Platform
│
├── Process
│
├── Metrics
│
├── Network
│
├── Events
│
├── Incidents
│
├── Policy
│
├── Storage
│
└── IPC
```

---

# 9. Platform Layer

The rest of the C++ engine should not directly depend everywhere on Mach/macOS APIs.

Instead create wrappers.

Example:

```cpp
namespace platform
{
    SystemCpuTimes readCpuTimes();
    MemoryStats readMemoryStats();
    std::vector<ProcessId> listProcesses();
}
```

Architecture:

```text
Metrics Engine
      │
      ▼
Platform Interface
      │
      ▼
macOS implementation
      │
      ▼
Mach / libproc / Apple APIs
```

This provides:

- testability
- clean architecture
- reduced platform coupling
- easier API replacement
- possibility of supporting Linux later

---

# 10. System Metrics Collector

Create:

```cpp
class SystemMetricsCollector
{
public:
    SystemSnapshot collect();

private:
    CpuCollector cpu_;
    MemoryCollector memory_;
    DiskCollector disk_;
};
```

Initial snapshot:

```cpp
struct SystemSnapshot
{
    Timestamp timestamp;

    CpuMetrics cpu;
    MemoryMetrics memory;
    DiskMetrics disk;
};
```

---

# 11. CPU Metrics

Initial CPU metrics:

```cpp
struct CpuMetrics
{
    double totalPercent;
    double userPercent;
    double systemPercent;
    double idlePercent;
};
```

Possible macOS implementation can use Mach host statistics.

Apple exposes APIs such as:

```text
host_statistics64()
```

for obtaining host statistics.

CPU calculation must use deltas.

Example:

```text
Snapshot T1

user    12000
system   4000
idle    30000

Snapshot T2

user    12200
system   4100
idle    30200
```

Calculate changes:

```text
userDelta   = 200
systemDelta = 100
idleDelta   = 200
```

Total:

```text
500 ticks
```

CPU utilisation:

```text
(200 + 100) / 500

= 60%
```

Do not calculate CPU usage using cumulative counters directly.

---

# 12. Memory Metrics

Initial structure:

```cpp
struct MemoryMetrics
{
    uint64_t totalBytes;
    uint64_t usedBytes;
    uint64_t freeBytes;

    uint64_t activeBytes;
    uint64_t inactiveBytes;
    uint64_t wiredBytes;
    uint64_t compressedBytes;
};
```

Later:

```text
Memory pressure
Swap
Page-ins
Page-outs
Compression
```

---

# 13. Process Registry

The system needs a persistent model of currently known processes.

```cpp
struct ProcessInfo
{
    ProcessId pid;
    ProcessId parentPid;

    std::string name;
    std::string executablePath;

    Timestamp firstSeen;
};
```

Manager:

```cpp
class ProcessRegistry
{
public:
    void update();

    const ProcessInfo* find(ProcessId pid) const;

    std::vector<ProcessInfo> processes() const;
};
```

Initial implementation may periodically enumerate processes.

Apple documentation references functions including:

```text
proc_pidinfo
proc_pidpath
proc_name
```

through `libproc`.

---

# 14. Process Metrics

Each tracked process should eventually maintain:

```cpp
struct ProcessMetrics
{
    ProcessId pid;

    double cpuPercent;

    uint64_t residentMemoryBytes;

    uint64_t bytesRead;
    uint64_t bytesWritten;

    uint64_t wakeups;
};
```

Network metrics will be integrated separately because attribution may use different APIs.

---

# 15. Process Tree

Watch should maintain parent-child relationships.

Example:

```text
Terminal
└── zsh
    └── python
        └── training.py
```

or:

```text
VS Code
├── Electron Helper
├── extension-host
│   └── language-server
└── git
```

This is important because simply reporting:

```text
python
```

is often insufficient.

A resource event should be attributable to its process ancestry.

---

# 16. Sampling Engine

Initial default:

```text
System metrics:       1 Hz
Process metrics:      1 Hz
```

Architecture:

```cpp
class SamplingEngine
{
public:
    void start();
    void stop();

private:
    std::jthread worker_;
};
```

Prefer:

```cpp
std::jthread
```

over manually managed thread lifetime when appropriate.

Worker behaviour:

```text
wake
 ↓
capture timestamp
 ↓
collect metrics
 ↓
update process registry
 ↓
create snapshot
 ↓
send snapshot to event engine
 ↓
persist
 ↓
sleep until next interval
```

The scheduling should use steady-clock timing to reduce drift.

---

# 17. Historical Metrics

Watch is fundamentally historical.

Current metrics alone are insufficient.

Raw snapshots should initially be stored locally.

Possible storage choice:

**SQLite**

Reasons:

- embedded
- zero external server
- mature
- efficient
- easy querying
- works well for metadata/time-series data at this project's scale

Initial tables:

```text
system_samples
process_samples
processes
events
incidents
network_flows
policy_rules
```

---

# 18. Database Schema — Initial Concept

## system_samples

```text
timestamp
cpu_total
cpu_user
cpu_system
memory_used
memory_free
disk_read_bytes
disk_write_bytes
```

## processes

```text
process_id
pid
parent_pid
name
executable_path
first_seen
last_seen
```

`process_id` should be an internal identifier.

Do not assume PID permanently identifies a process because macOS can reuse PIDs.

Potential process identity:

```text
PID + start time
```

---

## process_samples

```text
timestamp
process_id
cpu_percent
resident_memory
disk_read
disk_write
```

---

## events

```text
event_id
timestamp
type
severity
process_id
payload
```

---

## incidents

```text
incident_id
start_time
end_time
category
severity
summary
```

---

# 19. Data Retention

Do not store one-second metrics forever.

Use tiered retention.

Example:

```text
0–1 hour
1-second resolution

1–24 hours
10-second resolution

1–7 days
1-minute resolution

7–30 days
5-minute resolution
```

This creates an aggregation pipeline.

```text
Raw samples
     ↓
10-second aggregates
     ↓
1-minute aggregates
     ↓
5-minute aggregates
```

The system can periodically compact old data.

This becomes an advanced subproject of its own.

---

# 20. Event Engine

Snapshots represent measurements.

Events represent meaningful changes.

Examples:

```text
CPU_SPIKE
MEMORY_PRESSURE
HIGH_DISK_IO
PROCESS_STARTED
PROCESS_STOPPED
HIGH_NETWORK_UPLOAD
NETWORK_CHANGED
POLICY_TRIGGERED
FLOW_BLOCKED
```

Event structure:

```cpp
enum class EventType
{
    CpuSpike,
    MemoryPressure,
    DiskSpike,
    ProcessStarted,
    ProcessExited,
    NetworkChanged,
    PolicyTriggered
};
```

```cpp
struct SystemEvent
{
    EventId id;
    Timestamp timestamp;

    EventType type;
    Severity severity;

    std::optional<ProcessId> process;
};
```

---

# 21. Incident Engine

An event is one occurrence.

An incident groups related events.

Example:

```text
13:42:03
High network upload

13:42:06
High disk activity

13:42:08
CPU spike

13:42:17
Thermal state increased
```

Rather than presenting four independent alerts, create:

```text
Incident #104

Resource contention
13:42:03–13:48:41
```

Related processes:

```text
Dropbox
Docker
Chrome
```

---

# 22. Incident Detection V1

Do NOT start with machine learning.

Use deterministic thresholds.

Example:

```cpp
if (cpu.totalPercent > 85.0 &&
    duration > 20s)
{
    emit(CpuSpike);
}
```

Potential initial rules:

```text
CPU > 85% for 20 sec

Memory usage > 90%

Disk utilisation above threshold for 30 sec

Process CPU > 70% for 30 sec
```

Later move toward adaptive baselines.

---

# 23. Adaptive Detection — Future

Eventually calculate normal behaviour.

Example:

```text
Chrome typical CPU:

mean: 12%
std dev: 5%
```

Current:

```text
74%
```

Watch can identify:

```text
significant deviation from baseline
```

Potential algorithms:

```text
rolling average
EWMA
z-score
percentiles
seasonal baseline
```

Machine learning is not necessary initially.

---

# 24. Network Architecture

Network functionality should be separated from system monitoring.

Components:

```text
Network Monitor
      │
      ▼
Flow Registry
      │
      ▼
Policy Engine
      │
      ▼
Network Extension
```

Apple's content-filter architecture exposes `NEFilterFlow` objects representing application network connections and allows the provider to pass or block them.

---

# 25. Network Flow Model

Internal representation:

```cpp
struct NetworkFlow
{
    FlowId id;

    ProcessIdentity process;

    NetworkProtocol protocol;

    std::string remoteHost;

    uint16_t remotePort;

    Timestamp started;

    FlowDirection direction;
};
```

Potential protocol enum:

```cpp
enum class NetworkProtocol
{
    TCP,
    UDP,
    Unknown
};
```

---

# 26. Network Decision Model

Initial policy result:

```cpp
enum class NetworkDecision
{
    Allow,
    Block,
    Ask,
    Notify
};
```

Actual enforcement supported by the Network Extension layer should be separated from logical decisions.

```text
Policy Engine
     │
     ▼
NetworkDecision
     │
     ▼
Network Extension Adapter
     │
     ▼
Apple verdict
```

---

# 27. Policy Engine

Core C++ interface:

```cpp
class PolicyEngine
{
public:
    NetworkDecision evaluate(
        const NetworkFlow& flow,
        const SystemContext& context
    ) const;
};
```

Context:

```cpp
struct SystemContext
{
    NetworkProfile network;

    bool onBattery;

    double batteryLevel;

    bool lowPowerMode;

    SystemLoad load;
};
```

---

# 28. Policy Rule Model

Initial representation:

```cpp
struct PolicyRule
{
    RuleId id;

    ApplicationMatcher application;
    NetworkMatcher network;

    NetworkDecision decision;

    bool enabled;
};
```

Example:

```text
Application:
Steam

Network:
Hotspot

Action:
BLOCK
```

---

# 29. Profiles

Initial profiles:

```text
Default
Home
Work
Hotspot
Public Wi-Fi
```

Potential later profiles:

```text
Focus
Presentation
Development
Low Battery
Travel
```

Example:

```text
PROFILE: HOTSPOT

Safari           ALLOW
Terminal         ALLOW
VS Code          ALLOW
Zoom             ALLOW

Dropbox          BLOCK
Steam            BLOCK
OneDrive         BLOCK
Adobe Updater    BLOCK
```

---

# 30. Policy Language — Advanced Feature

Eventually develop a small DSL.

Example:

```text
profile "Hotspot" {

    allow app "Safari"
    allow app "Zoom"

    block app "Steam"
    block app "Dropbox"

    notify app "*"
        when upload > 500MB
}
```

Implementation:

```text
Source text
    ↓
Lexer
    ↓
Tokens
    ↓
Parser
    ↓
AST
    ↓
Rule compiler
    ↓
Policy Engine
```

This should NOT be part of the MVP.

Initially store structured rules directly.

---

# 31. Network Extension Layer

This portion will likely require Swift or Objective-C++ interoperability because Apple's Network Extension APIs are native Apple frameworks.

Conceptually:

```text
NEFilterDataProvider
        │
        ▼
Convert Apple flow
        │
        ▼
Internal FlowDescriptor
        │
        ▼
Policy Decision
        │
        ▼
Allow / Block verdict
```

Network filtering should remain isolated from the core engine.

---

# 32. Application Architecture

Recommended targets inside Xcode:

```text
Watch
│
├── WatchApp
│      SwiftUI
│
├── WatchCore
│      C++
│
├── WatchBridge
│      Objective-C++
│
├── WatchNetworkExtension
│      Swift / NetworkExtension
│
├── WatchCLI
│      C++
│
└── WatchTests
```

---

# 33. Why Objective-C++ Bridge?

Swift cannot directly consume arbitrary C++ APIs as cleanly as we want for a long-term modular architecture, despite improving C++ interoperability.

Use an explicit bridging layer initially.

Example:

```text
SwiftUI
   │
   ▼
WatchBridge
Objective-C++
   │
   ▼
WatchCore
C++
```

This provides a stable boundary.

---

# 34. Backend First

Do not start by making a beautiful GUI.

Initial development should happen through:

```text
WatchCLI
```

Example:

```bash
Watch status
```

Output:

```text
CPU        34.2%
Memory     8.41 / 16 GB
Processes  418

Disk Read     12.4 MB/s
Disk Write     4.7 MB/s
```

Then:

```bash
Watch top
```

```text
PID    PROCESS              CPU       MEMORY

4821   Chrome Helper        31.4%     712 MB
992    WindowServer         17.7%     1.2 GB
8911   clang                14.3%     340 MB
```

---

# 35. CLI Commands — Initial

Eventually:

```text
Watch status
Watch top
Watch ps
Watch incidents
Watch incident <id>
Watch network
Watch rules
Watch profiles
```

But only:

```text
status
top
ps
```

are necessary initially.

---

# 36. Repository Structure

Recommended:

```text
Watch/
│
├── README.md
├── PROJECT.md
├── LICENSE
│
├── app/
│   └── macos/
│
├── core/
│   │
│   ├── include/
│   │   └── Watch/
│   │
│   └── src/
│
│       ├── metrics/
│       ├── process/
│       ├── events/
│       ├── incidents/
│       ├── network/
│       ├── policy/
│       ├── storage/
│       └── platform/
│
├── platform/
│   └── macos/
│
├── bridge/
│
├── extensions/
│   └── network/
│
├── cli/
│
├── tests/
│   ├── unit/
│   ├── integration/
│   └── fixtures/
│
├── docs/
│   ├── architecture/
│   ├── decisions/
│   └── research/
│
└── tools/
```

---

# 37. Namespace

Everything in the core should use:

```cpp
namespace Watch
{
}
```

Subnamespaces:

```cpp
Watch::metrics
Watch::process
Watch::storage
Watch::policy
Watch::network
Watch::platform
```

---

# 38. C++ Standard

Use:

```text
C++20 minimum
```

C++23 features may be adopted selectively if supported cleanly by the target Xcode/Clang version.

Useful features include:

```text
std::jthread
std::stop_token
std::span
std::optional
std::variant
std::chrono
concepts
ranges
smart pointers
RAII
```

---

# 39. Memory Management Rules

Avoid raw owning pointers.

Prefer:

```cpp
std::unique_ptr
```

for exclusive ownership.

Use:

```cpp
std::shared_ptr
```

only when shared lifetime is truly required.

Non-owning references should usually be:

```cpp
T&
const T&
T*
std::span
```

depending on semantics.

---

# 40. Error Handling

Do not hide failure.

Potential model:

```cpp
std::expected<T, Error>
```

if toolchain support is sufficient.

Otherwise implement:

```cpp
Result<T>
```

or use explicit error types.

Avoid extensive exception usage across system boundaries.

---

# 41. Logging

Implement logging very early.

Levels:

```text
TRACE
DEBUG
INFO
WARN
ERROR
CRITICAL
```

Example:

```text
13:44:01 INFO  ProcessRegistry  discovered pid=4812 Chrome
13:44:02 WARN  IncidentEngine   CPU threshold crossed
13:44:07 INFO  IncidentEngine   incident=142 created
```

Logs will be essential when debugging extensions and background processes.

---

# 42. Threading Model

Initial backend:

```text
Main
 │
 ├── Sampling Thread
 │
 ├── Storage Worker
 │
 └── Event Worker
```

Eventually:

```text
Collector
    ↓
Concurrent Queue
    ↓
Processing Worker
    ↓
Storage Queue
    ↓
Database Worker
```

Avoid unnecessary threads initially.

One well-designed worker is better than premature concurrency.

---

# 43. Message Queue

Potential internal abstraction:

```cpp
template<typename T>
class BlockingQueue
{
public:
    void push(T value);

    std::optional<T> pop(std::stop_token stop);
};
```

This is a useful exercise in:

```text
mutex
condition_variable
move semantics
stop tokens
thread coordination
```

---

# 44. Testing Strategy

Three layers.

## Unit tests

Examples:

```text
CPU percentage calculation
process identity logic
policy matching
incident thresholds
aggregation
serialization
```

---

## Integration tests

Examples:

```text
collect actual CPU metrics
enumerate processes
write/read SQLite
launch process → discover process
terminate process → detect disappearance
```

---

## System tests

Examples:

```text
generate artificial CPU load
generate disk activity
generate network traffic
verify incident
```

Create helper programs.

Example:

```text
tools/
    cpu_stress
    memory_stress
    disk_stress
    network_test
```

This allows deterministic testing.

---

# 45. Performance Targets

Watch itself must not become the problem it monitors.

Initial targets:

```text
Idle CPU:
< 1%

Memory:
< 100 MB

Sampling overhead:
minimal

Database:
bounded growth
```

These are design targets rather than strict guarantees during early development.

---

# 46. Privacy Principles

Watch should follow:

```text
local-first
minimum required information
no telemetry by default
no network payload collection
transparent policy behaviour
```

Users should always understand why traffic was blocked.

Example:

```text
Blocked by:

Hotspot Profile
Rule #14

Application:
Steam

Condition:
Current network = Personal Hotspot
```

---

# 47. MVP Definition

Version `0.1` is **NOT the network-control version**.

V0.1 exists to establish the system engine.

It must accomplish:

```text
✓ Discover processes

✓ Measure system CPU

✓ Measure system memory

✓ Measure process CPU

✓ Measure process memory

✓ Measure basic disk activity

✓ Store historical samples

✓ Display current state via CLI

✓ Detect CPU incidents
```

If these work reliably, V0.1 is complete.

---

# 48. Version Roadmap

## V0.1 — System Core

```text
Process registry
CPU metrics
Memory metrics
Disk metrics
Sampling engine
SQLite
CLI
Basic incidents
```

---

## V0.2 — Historical Intelligence

```text
timeline queries
process history
incident correlation
retention system
aggregation
```

---

## V0.3 — Native macOS UI

```text
SwiftUI application

Dashboard
Processes
Timeline
Incidents
```

---

## V0.4 — Network Visibility

```text
Network Extension prototype
flow discovery
application mapping
destination display
flow history
```

---

## V0.5 — Network Policy

```text
allow rules
block rules
profiles
policy engine
policy auditing
```

---

## V0.6 — Context-Aware Policy

```text
network detection
profile switching
hotspot mode
battery-aware policies
notifications
```

---

## V1.0 — Watch

Expected feature set:

```text
System monitoring
Historical timeline
Process intelligence
Incident detection
Network visibility
Network policies
Profiles
Native macOS GUI
Local database
```

---

# 49. Development Order

The implementation should follow this sequence.

```text
1. Repository + Xcode project
        ↓
2. Platform abstraction
        ↓
3. CPU collector
        ↓
4. Memory collector
        ↓
5. Process enumeration
        ↓
6. Process metrics
        ↓
7. Sampling engine
        ↓
8. CLI
        ↓
9. SQLite
        ↓
10. Event engine
        ↓
11. Incident detector
        ↓
12. SwiftUI UI
        ↓
13. Network Extension prototype
        ↓
14. Flow registry
        ↓
15. Policy engine
```

Do **not** begin with Network Extension.

Build the observability engine first.

---

# 50. First Development Milestone

The first meaningful executable should produce this:

```bash
$ Watch
```

```text
Watch 0.1-dev

System
--------------------------------

CPU               27.8%
Memory             9.2 / 16.0 GB
Processes          436

Top Processes
--------------------------------

PID       PROCESS             CPU

8231      Chrome Helper       21.2%
7021      Xcode               14.6%
411       WindowServer         8.4%
8401      clang                5.7%
```

Refresh approximately every second.

That is Milestone 1.

---

# 51. First Classes to Implement

Create:

```text
Timestamp
ProcessId

CpuMetrics
MemoryMetrics
ProcessInfo
ProcessMetrics
SystemSnapshot
```

Then:

```text
CpuCollector
MemoryCollector
ProcessEnumerator
ProcessMetricsCollector
```

Then:

```text
SystemMonitor
```

---

# 52. Proposed Initial Interfaces

```cpp
#pragma once

namespace Watch::metrics
{

struct CpuMetrics
{
    double totalPercent {};
    double userPercent {};
    double systemPercent {};
    double idlePercent {};
};

class CpuCollector
{
public:
    CpuCollector();

    CpuMetrics sample();

private:
    // previous Mach CPU counters
};

}
```

Memory:

```cpp
class MemoryCollector
{
public:
    MemoryMetrics sample() const;
};
```

Process:

```cpp
class ProcessEnumerator
{
public:
    std::vector<ProcessInfo> enumerate() const;
};
```

Main monitor:

```cpp
class SystemMonitor
{
public:
    SystemSnapshot sample();

private:
    CpuCollector cpuCollector_;
    MemoryCollector memoryCollector_;
    ProcessEnumerator processEnumerator_;
};
```

---

# 53. First Xcode Configuration

Create an Xcode project containing:

```text
WatchCore
WatchCLI
WatchTests
```

Initially ignore:

```text
WatchApp
NetworkExtension
Bridge
```

They can be added later.

The first development loop should be entirely:

```text
C++
+
Xcode debugger
+
Terminal output
```

---

# 54. First Sprint

## Task 1

Create repository structure.

## Task 2

Configure C++20.

## Task 3

Create `WatchCore`.

## Task 4

Create CPU metric structures.

## Task 5

Implement macOS CPU collection.

## Task 6

Calculate CPU deltas correctly.

## Task 7

Implement memory collection.

## Task 8

Enumerate running processes.

## Task 9

Create CLI output.

## Task 10

Refresh metrics continuously.

At the end:

```text
Watch CLI system monitor working.
```

Do not introduce SQLite yet.

---

# 55. Second Sprint

Add:

```text
per-process CPU
per-process memory
process parent relationships
sorting
top command
```

Result:

```bash
Watch top
```

---

# 56. Third Sprint

Add SQLite.

Persist:

```text
system samples
processes
process samples
```

Then implement:

```bash
Watch history
```

---

# 57. Fourth Sprint

Create:

```text
EventEngine
IncidentDetector
```

First incident:

```text
CPU_SPIKE
```

Generate artificial CPU load and validate detection.

---

# 58. Fifth Sprint

Create the first SwiftUI interface.

Pages:

```text
Overview

Processes

Timeline

Incidents
```

Nothing related to networking yet.

---

# 59. Network Prototype Milestone

Only after system monitoring works:

Create a standalone test Network Extension.

Purpose:

```text
Can Watch observe a flow?

Can it identify the originating application?

Can it allow the flow?

Can it block the flow?

Can rules be updated reliably?
```

Do not immediately integrate this deeply into the main application.

Treat it as an experimental milestone.

---

# 60. Network Extension Proof-of-Concept

Required demonstration:

```text
Safari:
ALLOW

Test application:
BLOCK
```

Then verify:

```text
test app connection fails

Safari connection succeeds
```

If successful, proceed to policy integration.

---

# 61. Policy Engine Prototype

Do not initially build profiles.

Hard-code:

```cpp
Rule rule {
    .application = "TestApplication",
    .decision = NetworkDecision::Block
};
```

Then migrate to:

```text
SQLite rules
```

Then profiles.

---

# 62. Major Technical Risks

## Risk 1 — macOS API restrictions

Some desired system information may not be accessible exactly as envisioned.

Mitigation:

```text
prototype every privileged capability independently
before designing functionality around it.
```

---

## Risk 2 — Network attribution

Mapping all traffic cleanly to applications may have edge cases.

Mitigation:

Create a dedicated Network Extension proof-of-concept before committing to V0.4 architecture.

---

## Risk 3 — Endpoint Security entitlement

Endpoint Security requires Apple approval.

Mitigation:

Do not make it a V1 dependency.

---

## Risk 4 — Database size

One-second process samples across hundreds of processes can grow rapidly.

Mitigation:

Aggregation + retention.

---

## Risk 5 — Monitoring overhead

Aggressive process polling may itself consume resources.

Mitigation:

Benchmark collectors and adapt sampling intervals.

---

# 63. Architectural Principle

The most important rule for this project:

> **The core should describe what Watch wants to know, not how macOS happens to provide it.**

Bad:

```cpp
class CpuMonitor
{
    mach_msg_type_number_t count;
    host_cpu_load_info_data_t cpuInfo;
};
```

spread throughout the application.

Better:

```text
Watch
      │
      ▼
ICpuProvider
      │
      ▼
MacCpuProvider
      │
      ▼
Mach APIs
```

macOS-specific implementation remains behind platform boundaries.

---

# 64. Learning Goals

This project should develop practical experience with:

### Modern C++

```text
RAII
smart pointers
move semantics
templates
concepts
ranges
std::chrono
std::jthread
atomics
mutexes
condition variables
filesystem
variants
optionals
error handling
```

### Operating systems

```text
processes
process trees
resource accounting
CPU measurement
virtual memory concepts
system calls
IPC
event-driven systems
daemons
resource scheduling
```

### macOS internals

```text
Mach APIs
libproc
Network Extension
System Extensions
Xcode capabilities
code signing
entitlements
Swift/C++ interoperability
```

### Systems architecture

```text
collectors
event pipelines
time-series storage
rule engines
state machines
background services
concurrent queues
observability
```

### Networking

```text
TCP/UDP flows
endpoints
application attribution
network policies
connection filtering
```

---

# 65. What Makes Watch Worth Building

The project should not be judged by:

```text
"Can I build Activity Monitor?"
```

Activity Monitor already exists.

The actual differentiator is:

```text
Current system state
        +
Historical state
        +
Process relationships
        +
Incident detection
        +
Network visibility
        +
Context-aware policies
```

The eventual user experience should answer:

```text
My Mac was slow 20 minutes ago.

Why?
```

instead of merely:

```text
My Mac is currently using 35% CPU.
```

And:

```text
I'm on my phone hotspot.

Which applications should be allowed to communicate?
```

instead of requiring the user to manually shut down background applications.

---

# 66. Success Criteria

The project is successful when Watch can reliably demonstrate this scenario:

```text
1. User works normally.

2. Watch records resource history.

3. An application begins heavy activity.

4. CPU/network/disk usage spikes.

5. Watch identifies the responsible processes.

6. Watch creates an incident.

7. User opens Watch later.

8. User sees exactly when the event occurred.

9. User sees which applications contributed.

10. Watch explains the likely relationship.

11. User switches to a constrained network.

12. A network policy becomes active.

13. Selected background applications are prevented
    from creating network traffic.

14. Watch records why each policy was applied.
```

That is the target product.

---

# 67. Immediate Next Step

Do not start with:

```text
SwiftUI
Network Extension
SQLite
Policy Engine
ML
```

Start with:

```text
WatchCore
     │
     ├── CpuCollector
     ├── MemoryCollector
     └── ProcessEnumerator
```

and make this work:

```bash
$ Watch

CPU:       24.8%
Memory:    8.7 / 16 GB
Processes: 421
```

Then build upward.

The first engineering question should therefore be:

> **How do we design and implement `CpuCollector` correctly on macOS while keeping the rest of Watch independent of Mach?**

That is where implementation begins.
