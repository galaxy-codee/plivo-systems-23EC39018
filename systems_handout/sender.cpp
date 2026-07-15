#include <iostream>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

struct Frame {
    uint32_t seq;
    unsigned char payload[160];
};

int main() {
    // 1. Source Socket (Read incoming Clean Frames from Harness)
    int source_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in source_addr{};
    source_addr.sin_family = AF_INET;
    source_addr.sin_port = htons(47010);
    source_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(source_fd, (struct sockaddr *)&source_addr, sizeof(source_addr)) < 0) {
        perror("bind 47010");
        return 1;
    }

    // 2. Relay Socket (Send Redundant Frames to Flaky Network)
    int relay_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay_addr{};
    relay_addr.sin_family = AF_INET;
    relay_addr.sin_port = htons(47001);
    relay_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    unsigned char rx_buf[2048];
    Frame prev_frame{};
    bool has_prev = false;

    while (true) {
        ssize_t n = recvfrom(source_fd, rx_buf, sizeof(rx_buf), 0, nullptr, nullptr);
        if (n < 164) continue; // Malformed packet

        // Parse current frame (Harness sends Big-Endian Seq + 160-byte payload)
        uint32_t curr_seq;
        std::memcpy(&curr_seq, rx_buf, 4);
        curr_seq = ntohl(curr_seq);

        Frame curr_frame;
        curr_frame.seq = curr_seq;
        std::memcpy(curr_frame.payload, rx_buf + 4, 160);

        // Pack both current and previous frame (FEC) into a single 328-byte packet
        unsigned char tx_buf[328];
        
        // Write Current Frame
        uint32_t curr_seq_be = htonl(curr_frame.seq);
        std::memcpy(tx_buf, &curr_seq_be, 4);
        std::memcpy(tx_buf + 4, curr_frame.payload, 160);

        // Write Previous Frame
        uint32_t prev_seq_be = htonl(has_prev ? prev_frame.seq : 0xFFFFFFFF);
        std::memcpy(tx_buf + 164, &prev_seq_be, 4);
        std::memcpy(tx_buf + 168, prev_frame.payload, 160);

        // Send out to the Relay
        sendto(relay_fd, tx_buf, 328, 0, (struct sockaddr *)&relay_addr, sizeof(relay_addr));

        // Shift frames
        prev_frame = curr_frame;
        has_prev = true;
    }
    return 0;
}
