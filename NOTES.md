Our design solves the flaky network challenge by combining Forward Error Correction (FEC) and a high-resolution, decoupled jitter buffer. 
By piggybacking the previous frame ($N-1$) onto the current packet ($N$), we achieve zero-latency packet loss recovery for isolated drops.
The overall packet size is kept strictly at 328 bytes, which cleanly sits under the 2.0x bandwidth budget without needing an expensive back-and-forth ARQ feedback round-trip.
On the receiver side, a high-performance network reading thread instantly unpacks both frames and stores them in a synchronized hash map.
A separate playout thread starts at the exact, target-offset epoch clock ($T0 + \text{DELAY\_MS}$) and wakes up strictly every 20 milliseconds to transmit frames to the player.
This architecture eliminates reordering issues and absorbs delay jitter.
We recommend grading at a delay of 40ms, which safely handles loss and jitter across various profiles.
A burst drop of two or more consecutive packets will break this specific 1-frame redundant setup.
