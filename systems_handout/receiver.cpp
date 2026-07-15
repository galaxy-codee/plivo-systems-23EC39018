#include <iostream>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// Environment settings populated by the harness
double T0 = 0.0;
int DURATION_S = 0;
int DELAY_MS = 0;

// Thread-safe Frame Storage
struct Frame {
    bool present = false;
    unsigned char payload[160];
};

std::unordered_map<uint32_t, Frame> jitter_buffer;
std::mutex buffer_mutex;

// Packet Receiver Loop (Ingest Thread)
void receiver_loop(int in_fd) {
    unsigned char rx_buf[2048];
    while (true) {
        ssize_t n = recvfrom(in_fd, rx_buf, sizeof(rx_buf), 0, nullptr, nullptr);
        if (n < 328) continue; // Expecting our redundant 328-byte packet

        // Extract Current Frame
        uint32_t curr_seq;
        std::memcpy(&curr_seq, rx_buf, 4);
        curr_seq = ntohl(curr_seq);

        // Extract Previous Frame
        uint32_t prev_seq;
        std::memcpy(&prev_seq, rx_buf + 164, 4);
        prev_seq = ntohl(prev_seq);

        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            
            // Save current frame
            if (jitter_buffer.find(curr_seq) == jitter_buffer.end()) {
                Frame& f = jitter_buffer[curr_seq];
                f.present = true;
                std::memcpy(f.payload, rx_buf + 4, 160);
            }

            // Save previous frame (if valid)
            if (prev_seq != 0xFFFFFFFF && jitter_buffer.find(prev_seq) == jitter_buffer.end()) {
                Frame& f = jitter_buffer[prev_seq];
                f.present = true;
                std::memcpy(f.payload, rx_buf + 168, 160);
            }
        }
    }
}

// Playout Loop (Clock-Controlled Playback Thread)
void playout_loop(int out_fd, struct sockaddr_in player_addr) {
    // Calculate Playback Start Time from T0
    auto t0_system = std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(
            std::chrono::duration<double>(T0)
        )
    );
    
    auto start_playout = t0_system + std::chrono::milliseconds(DELAY_MS);

    // Synchronize playout start
    std::this_thread::sleep_until(start_playout);

    uint32_t playout_idx = 0;
    auto next_tick = start_playout;

    while (true) {
        Frame play_frame{};
        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            auto it = jitter_buffer.find(playout_idx);
            if (it != jitter_buffer.end()) {
                play_frame = it->second;
            }
        }

        // Construct 164-byte packet for the harness player
        unsigned char tx_buf[164];
        uint32_t seq_be = htonl(playout_idx);
        std::memcpy(tx_buf, &seq_be, 4);

        if (play_frame.present) {
            std::memcpy(tx_buf + 4, play_frame.payload, 160);
        } else {
            // Fill with silent bytes (dummy payload) if dropped/late
            std::memset(tx_buf + 4, 0, 160);
        }

        sendto(out_fd, tx_buf, 164, 0, (struct sockaddr *)&player_addr, sizeof(player_addr));

        playout_idx++;
        next_tick += std::chrono::milliseconds(20); // Steady 20ms clock
        std::this_thread::sleep_until(next_tick);
    }
}

int main() {
    // Read environments variables or fallback to safe defaults
    if (const char* env_t0 = std::getenv("T0")) T0 = std::stod(env_t0);
    if (const char* env_dur = std::getenv("DURATION_S")) DURATION_S = std::atoi(env_dur);
    if (const char* env_del = std::getenv("DELAY_MS")) DELAY_MS = std::atoi(env_del);

    // Sockets setup
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in in_addr{};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47002);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof(in_addr)) < 0) {
        perror("bind 47002");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in player_addr{};
    player_addr.sin_family = AF_INET;
    player_addr.sin_port = htons(47020);
    player_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Spawn Threads
    std::thread rx_thread(receiver_loop, in_fd);
    std::thread tx_thread(playout_loop, out_fd, player_addr);

    rx_thread.join();
    tx_thread.join();

    return 0;
}
