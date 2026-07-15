# Run Log - The Flaky Network

### Experiment 1: Baseline C Code
*   **Profile:** profiles/A.json
*   **Playout Delay (`delay_ms`):** 40 ms
*   **Miss Rate %:** ~15% - 25% (Highly variable)
*   **Bandwidth Overhead:** 1.0x
*   **Modifications:** None (Initial baseline)
*   **Result:** INVALID (Miss rate exceeds 1% threshold due to unrecovered packets)

### Experiment 2: FEC with 1-Frame Redundancy (Initial Test)
*   **Profile:** profiles/A.json
*   **Playout Delay (`delay_ms`):** 40 ms
*   **Miss Rate %:** 0.0%
*   **Bandwidth Overhead:** 1.97x (Within the 2.0x limit)
*   **Modifications:** Added 1-packet lookback redundant packaging (328 bytes total). Decoupled receiver into packet reading thread and clock-based scheduling playback thread.
*   **Result:** VALID

### Experiment 3: Minimizing Delay on Profile B
*   **Profile:** profiles/B.json
*   **Playout Delay (`delay_ms`):** 30 ms
*   **Miss Rate %:** 0.1%
*   **Bandwidth Overhead:** 1.97x
*   **Modifications:** Tuned the playout scheduler sleep cycles down to optimize response times under moderate jitter profiles.
*   **Result:** VALID (Optimized)
