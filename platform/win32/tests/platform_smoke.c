/*
 * Checks Windows portability primitives without pretending that the
 * full OB-UDPST client has been ported or tested.
 */
#include "../udpst_win32_platform.h"
#include <stdio.h>

int main(void)
{
    SOCKET sock = INVALID_SOCKET;
    uint64_t utc_ns = 0, before_ns = 0, after_ns = 0;
    int result = 1;

    if (udpst_win32_net_init() != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "UDP socket creation failed: %d\n", WSAGetLastError());
        goto done;
    }
    if (udpst_win32_set_nonblocking(sock) != 0) {
        fprintf(stderr, "ioctlsocket failed: %d\n", WSAGetLastError());
        goto done;
    }
    if (udpst_win32_realtime_ns(&utc_ns) != 0 || utc_ns == 0) {
        fprintf(stderr, "UTC clock failed\n");
        goto done;
    }
    if (udpst_win32_monotonic_ns(&before_ns) != 0 ||
        udpst_win32_monotonic_ns(&after_ns) != 0 ||
        after_ns < before_ns) {
        fprintf(stderr, "Monotonic clock failed\n");
        goto done;
    }
    puts("Windows socket and clock primitives: PASS");
    result = 0;
done:
    if (sock != INVALID_SOCKET)
        closesocket(sock);
    udpst_win32_net_cleanup();
    return result;
}
