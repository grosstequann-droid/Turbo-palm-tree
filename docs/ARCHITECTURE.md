## TurboPalmTree Architecture

A real-time, deterministic control system built on hardware-in-the-loop (HIL) principles.

### Core Philosophy

This is **not a script**. It's a production-grade control system that beats amateur approaches by:

1. **Eliminating latency sources** (direct capture, kernel I/O)
2. **Predicting state** instead of reacting to it
3. **Parallelizing everything** (lock-free pipeline)
4. **Measuring everything** (deterministic timing)

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  PCIe Capture Card                                              │
│  (HDMI → Uncompressed Video)                                    │
│                                                                 │
└────────────────┬────────────────────────────────────────────────┘
                 │
         ┌───────▼──────────────┐
         │  Thread 1: Capture   │ (Core 0, RT)
         │  ↓                   │
         │ Ring Buffer (x4)     │ ← Zero-copy, lock-free
         │                      │
         └──────────┬───────────┘
                    │
         ┌──────────▼──────────────┐
         │ Thread 2: Vision/CUDA   │ (Core 1, RT)
         │ ↓                       │
         │ OpenCV Detection        │
         │ ↓                       │
         │ Kalman Filter State     │
         │                         │
         └──────────┬──────────────┘
                    │
         ┌──────────▼────────────────┐
         │ Thread 3: Prediction      │ (Core 2, RT)
         │ ↓                         │
         │ Extrapolate State         │
         │ Calculate Trigger Time    │
         │ (10-20ms lookahead)       │
         │                           │
         └──────────┬────────────────┘
                    │
         ┌──────────▼────────────────┐
         │ Thread 4: I/O             │ (Core 3, RT)
         │ ↓                         │
         │ Wait for Precise Time     │
         │ ↓                         │
         │ ViGEmBus Kernel Driver    │
         │ ↓                         │
         │ Xbox Controller Emulation │
         │                           │
         └──────────┬────────────────┘
                    │
         ┌──────────▼────────────────┐
         │ Console (Game)            │
         │ (Sees native controller)  │
         │                           │
         └───────────────────────────┘
```

---

## Phase Breakdown

### Phase 1: Direct Capture (Foundation)
**Goal**: Get frames from PCIe card into memory with <1ms latency

- PCIe capture card driver integration
- Ring buffer management (lock-free, zero-copy)
- Frame synchronization
- Latency profiling

**Output**: Console app that prints captured frame metrics

### Phase 2: Vision Pipeline (State)
**Goal**: Detect meter position and estimate state

- OpenCV color space detection
- CUDA-accelerated processing
- Bounding box extraction
- Kalman filter state tracking

**Output**: Real-time meter position tracking

### Phase 3: Prediction & Control Logic
**Goal**: Predict future state and schedule input timing

- Kalman filter extrapolation (10-20ms lookahead)
- Green zone detection
- Trigger timing calculation
- Signal jitter analysis

**Output**: Precise trigger commands with <±2ms accuracy

### Phase 4: Kernel I/O Bridge
**Goal**: Inject input at precise timing with zero latency variance

- ViGEmBus integration
- Xbox controller emulation
- High-precision wait loops
- Jitter compensation

**Output**: Deterministic input injection (<1ms jitter)

---

## Key Design Decisions

### 1. Lock-Free Ring Buffer
```cpp
// Why: Eliminates lock contention between capture and vision threads
// How: Single producer (capture) → Single consumer (vision)
// Result: Guaranteed <1µs acquisition time
```

### 2. Real-Time Thread Priority
```cpp
// Why: Ensures predictable scheduling (no kernel context switches)
// How: 
//   - Windows: THREAD_PRIORITY_TIME_CRITICAL
//   - Linux: SCHED_FIFO with elevated priority
// Result: Deterministic latency (<1ms variance)
```

### 3. CPU Core Affinity
```cpp
// Why: Prevents thread migration (cache coherency, NUMA effects)
// How: Pin each thread to dedicated core
//   Thread 1 → Core 0
//   Thread 2 → Core 1
//   Thread 3 → Core 2
//   Thread 4 → Core 3
// Result: Predictable inter-thread communication
```

### 4. Kalman Filter Prediction
```cpp
// Why: Predicts meter state before it happens (offset latency)
// How:
//   1. Vision thread estimates [position, velocity]
//   2. Prediction thread extrapolates to future (10-20ms)
//   3. I/O thread triggers early, accounting for signal delay
// Result: Early trigger = natural input timing
```

### 5. Nanosecond-Precision Timing
```cpp
// Why: Sub-microsecond jitter is the difference between success/failure
// How:
//   - Use `std::chrono::high_resolution_clock`
//   - Busy-wait loops with CPU pause instructions
//   - No OS sleep (too coarse-grained)
// Result: <1µs timing accuracy
```

---

## Performance Targets

| Component | Target | Reason |
|-----------|--------|--------|
| Capture Latency | <1 ms | Direct HDMI feed, minimal processing |
| Vision Processing | 5-10 ms | CUDA acceleration, fixed image size |
| Prediction Math | 1-2 ms | Simple matrix math (Kalman) |
| I/O Jitter | <1 ms | Kernel-level, high-priority thread |
| **Total System Latency** | **<20 ms** | Offset signal delay to console |

---

## Thread Synchronization Strategy

**NO LOCKS. Lock-free everywhere.**

### Communication Mechanism
```
Capture Thread
    ↓ (writes)
Ring Buffer
    ↓ (reads)
Vision Thread
    ↓ (writes)
Atomic<MeterState>
    ↓ (reads)
Prediction Thread
    ↓ (writes)
Atomic<ControlSignal>
    ↓ (reads)
I/O Thread
```

### Why Lock-Free?
- Locks cause **priority inversion** (low-priority thread blocks high-priority thread)
- Unpredictable wait times (fatal in real-time systems)
- Atomic operations + busy-wait is deterministic

---

## Latency Budget Analysis

```
Console outputs frame
    ↓ (HDMI cable delay: ~1µs)
PCIe Card receives
    ↓ (DMA to RAM: ~100µs)
Capture Thread reads
    ↓ (write to ring buffer: ~1µs)
Vision Thread consumes
    ↓ (OpenCV detection: 5-10ms)
Kalman Filter updates
    ↓ (1-2ms math)
Prediction Thread extrapolates
    ↓ (I/O thread acquires signal: <1µs)
Kernel driver injects input
    ↓ (signal to console: ~5ms typical)
Game processes input
    ↓ (game logic: variable)
Game displays result
    ↓
TOTAL: ~11-20ms (depends on game)

BUT: We predict 10-20ms ahead, so trigger at T-10ms
Result: Natural input timing (indistinguishable from human player)
```

---

## Failure Modes & Detection

### 1. Ring Buffer Overrun
- **Symptom**: Vision thread falls behind
- **Detection**: Monitor buffer utilization
- **Recovery**: Log warning, continue (drop frames)

### 2. Prediction Jitter
- **Symptom**: Trigger timing variance >2ms
- **Detection**: Track input timestamp variance
- **Recovery**: Auto-tune Kalman filter parameters

### 3. Lost Meter Tracking
- **Symptom**: Vision thread loses meter (e.g., screen flash)
- **Detection**: Confidence metric drops
- **Recovery**: Hold last prediction, signal UI

---

## Future Optimizations

1. **GPU Ring Buffer** (DMA directly to GPU memory)
2. **Particle Filter** (multi-hypothesis tracking)
3. **Machine Learning** (trained meter detection)
4. **Telemetry Dashboard** (real-time latency visualization)
