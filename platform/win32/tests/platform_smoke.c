/*
 * Windows portability smoke test: sockets, UDP loopback and clocks.
 * This is NOT a TR-471 protocol interoperability test.
 */
#include "../udpst_win32_platform.h"
#include <stdio.h>
#include <string.h>

static int udp_loopback_ipv4(void)
{
    SOCKET receiver = INVALID_SOCKET, sender = INVALID_SOCKET;
    struct sockaddr_in address;
    int addrlen = (int)sizeof(address), timeout_ms = 2000;
    const char payload[] = "udpst-win32-loopback";
    char buffer[sizeof(payload)];
    int rc = -1;

    receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sender = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiver == INVALID_SOCKET || sender == INVALID_SOCKET)
        goto done;

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(0);
    if (bind(receiver, (const struct sockaddr *)&address, addrlen) == SOCKET_ERROR ||
        getsockname(receiver, (struct sockaddr *)&address, &addrlen) == SOCKET_ERROR)
        goto done;

    if (setsockopt(receiver, SOL_SOCKET, SO_RCVTIMEO,
                   (const char *)&timeout_ms, sizeof(timeout_ms)) == SOCKET_ERROR)
        goto done;

    if (udpst_win32_wait_readable(receiver, 0) != 0)
        goto done;
    {
        WSAPOLLFD entries[2];
        memset(entries, 0, sizeof(entries));
        entries[0].fd = receiver;
        entries[0].events = POLLRDNORM;
        entries[1].fd = sender;
        entries[1].events = POLLWRNORM;
        if (udpst_win32_poll_udp(entries, 2, 1000) < 1 ||
            !(entries[1].revents & POLLWRNORM) ||
            (entries[0].revents & POLLRDNORM))
            goto done;
    }
    if (udpst_win32_wait_writable(sender, 1000) != 1)
        goto done;
    if (sendto(sender, payload, (int)sizeof(payload), 0,
               (const struct sockaddr *)&address, addrlen) != (int)sizeof(payload))
        goto done;
    {
        WSAPOLLFD entries[2];
        memset(entries, 0, sizeof(entries));
        entries[0].fd = receiver;
        entries[0].events = POLLRDNORM;
        entries[1].fd = sender;
        entries[1].events = POLLRDNORM;
        if (udpst_win32_poll_udp(entries, 2, 1000) < 1 ||
            !(entries[0].revents & POLLRDNORM) ||
            (entries[1].revents & POLLRDNORM))
            goto done;
    }
    {
        udpst_win32_poll_entry entries[2];
        unsigned int ready_indices[2] = {0, 0};
        entries[0].socket_handle = receiver;
        entries[0].connection_index = 17;
        entries[1].socket_handle = sender;
        entries[1].connection_index = 23;
        if (udpst_win32_poll_connections(entries, 2, 1000, ready_indices, 2) != 1 ||
            ready_indices[0] != 17)
            goto done;
    }
    if (udpst_win32_wait_readable(receiver, 1000) != 1)
        goto done;
    if (recvfrom(receiver, buffer, (int)sizeof(buffer), 0, NULL, NULL) !=
        (int)sizeof(payload))
        goto done;
    if (memcmp(buffer, payload, sizeof(payload)) != 0)
        goto done;
    if (udpst_win32_wait_readable(receiver, 0) != 0)
        goto done;
    rc = 0;
done:
    if (receiver != INVALID_SOCKET) closesocket(receiver);
    if (sender != INVALID_SOCKET) closesocket(sender);
    return rc;
}

int main(void)
{
    SOCKET sock = INVALID_SOCKET, sock6 = INVALID_SOCKET;
    struct addrinfo *resolved = NULL, *node;
    int found_ipv4 = 0;
    uint64_t utc_ns = 0, before_ns = 0, after_ns = 0;
    int result = 1;

    if (udpst_win32_net_init() != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "IPv4 UDP socket creation failed: %d\n", WSAGetLastError());
        goto done;
    }
    if (udpst_win32_set_nonblocking(sock) != 0) {
        fprintf(stderr, "ioctlsocket failed: %d\n", WSAGetLastError());
        goto done;
    }
    sock6 = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock6 == INVALID_SOCKET) {
        fprintf(stderr, "IPv6 UDP socket creation failed: %d\n", WSAGetLastError());
        goto done;
    }
    if (udpst_win32_set_nonblocking(sock6) != 0) {
        fprintf(stderr, "IPv6 ioctlsocket failed: %d\n", WSAGetLastError());
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
    if (udpst_win32_resolve_udp("127.0.0.1", "24601", &resolved) != 0) {
        fprintf(stderr, "IPv4 UDP destination resolution failed\\n");
        goto done;
    }
    for (node = resolved; node != NULL; node = node->ai_next) {
        if (node->ai_family == AF_INET &&
            ntohs(((const struct sockaddr_in *)node->ai_addr)->sin_port) == 24601)
            found_ipv4 = 1;
    }
    if (!found_ipv4) {
        fprintf(stderr, "Resolved IPv4 endpoint missing expected port\\n");
        goto done;
    }
    freeaddrinfo(resolved);
    resolved = NULL;
    if (udp_loopback_ipv4() != 0) {
        fprintf(stderr, "IPv4 UDP loopback failed: %d\n", WSAGetLastError());
        goto done;
    }
    puts("Windows IPv4/IPv6 sockets, select readiness, UDP resolution, IPv4 UDP loopback and clocks: PASS");
    result = 0;
done:
    if (resolved != NULL) freeaddrinfo(resolved);
    if (sock6 != INVALID_SOCKET) closesocket(sock6);
    if (sock != INVALID_SOCKET) closesocket(sock);
    udpst_win32_net_cleanup();
    return result;
}
