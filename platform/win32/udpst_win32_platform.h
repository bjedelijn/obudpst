/*
 * Windows client portability primitives for OB-UDPST.
 *
 * This header is deliberately not wired into the existing POSIX sources yet.
 * The Windows event loop and socket lifetime must be converted together:
 * SOCKET is pointer-sized and must never be silently truncated to int.
 *
 * Minimum Windows version: Windows Server 2012 (_WIN32_WINNT=0x0602).
 */
#ifndef UDPST_WIN32_PLATFORM_H
#define UDPST_WIN32_PLATFORM_H

#if !defined(_WIN32)
#error "udpst_win32_platform.h is only for Windows builds"
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>

/*
 * ECN uses the low two bits of the IPv4 TOS / IPv6 Traffic Class octet
 * (RFC 3168). These numeric protocol values are portable; socket ancillary
 * message support still requires a separate Windows implementation.
 */
#ifndef IPTOS_ECN
#define IPTOS_ECN(x) ((x) & 0x03)
#endif
#ifndef IPTOS_ECN_NOT_ECT
#define IPTOS_ECN_NOT_ECT 0x00
#endif
#ifndef IPTOS_ECN_ECT1
#define IPTOS_ECN_ECT1 0x01
#endif
#ifndef IPTOS_ECN_ECT0
#define IPTOS_ECN_ECT0 0x02
#endif
#ifndef IPTOS_ECN_CE
#define IPTOS_ECN_CE 0x03
#endif

/* Initialize once before any socket operation; call cleanup on shutdown. */
static int udpst_win32_net_init(void)
{
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0 ? 0 : -1;
}

static void udpst_win32_net_cleanup(void)
{
    WSACleanup();
}

static int udpst_win32_set_nonblocking(SOCKET socket_handle)
{
    u_long enabled = 1;
    return ioctlsocket(socket_handle, FIONBIO, &enabled) == 0 ? 0 : -1;
}

/* Wall-clock UTC time, suitable for protocol timestamps (not elapsed time). */
static int udpst_win32_realtime_ns(uint64_t *ns)
{
    FILETIME ft;
    ULARGE_INTEGER ticks;
    const uint64_t epoch_offset_100ns = UINT64_C(116444736000000000);
    if (ns == NULL) return -1;
    GetSystemTimeAsFileTime(&ft);
    ticks.LowPart = ft.dwLowDateTime;
    ticks.HighPart = ft.dwHighDateTime;
    if (ticks.QuadPart < epoch_offset_100ns) return -1;
    *ns = (ticks.QuadPart - epoch_offset_100ns) * UINT64_C(100);
    return 0;
}

/* Monotonic interval measurement; independent of wall-clock adjustments. */
static int udpst_win32_monotonic_ns(uint64_t *ns)
{
    LARGE_INTEGER count, frequency;
    if (ns == NULL || !QueryPerformanceFrequency(&frequency) ||
        !QueryPerformanceCounter(&count) || frequency.QuadPart <= 0)
        return -1;
    /* Avoid overflow when the counter has run for a long time. */
    *ns = ((uint64_t)(count.QuadPart / frequency.QuadPart) *
           UINT64_C(1000000000)) +
          ((uint64_t)(count.QuadPart % frequency.QuadPart) *
           UINT64_C(1000000000) / (uint64_t)frequency.QuadPart);
    return 0;
}


/*
 * Wait for a UDP socket to become readable. select() accepts SOCKET values
 * directly on Windows, unlike POSIX integer file descriptors.
 * Returns 1 (ready), 0 (timeout), or -1 (socket error).
 */
static int udpst_win32_wait_readable(SOCKET socket_handle, unsigned int timeout_ms)
{
    fd_set read_set;
    struct timeval timeout;
    int ready;
    if (socket_handle == INVALID_SOCKET)
        return -1;
    FD_ZERO(&read_set);
    FD_SET(socket_handle, &read_set);
    timeout.tv_sec = (long)(timeout_ms / 1000U);
    timeout.tv_usec = (long)((timeout_ms % 1000U) * 1000U);
    ready = select(0, &read_set, NULL, NULL, &timeout);
    if (ready == SOCKET_ERROR)
        return -1;
    return ready > 0 && FD_ISSET(socket_handle, &read_set) ? 1 : 0;
}

/*
 * Equivalent wait for a socket to become writable. This is a foundation
 * for the future Windows event loop; it is not an epoll replacement yet.
 */
static int udpst_win32_wait_writable(SOCKET socket_handle, unsigned int timeout_ms)
{
    fd_set write_set;
    struct timeval timeout;
    int ready;
    if (socket_handle == INVALID_SOCKET)
        return -1;
    FD_ZERO(&write_set);
    FD_SET(socket_handle, &write_set);
    timeout.tv_sec = (long)(timeout_ms / 1000U);
    timeout.tv_usec = (long)((timeout_ms % 1000U) * 1000U);
    ready = select(0, NULL, &write_set, NULL, &timeout);
    if (ready == SOCKET_ERROR)
        return -1;
    return ready > 0 && FD_ISSET(socket_handle, &write_set) ? 1 : 0;
}


/*
 * Wait for multiple UDP sockets without narrowing SOCKET to int.
 * The caller retains the WSAPOLLFD array; revents are updated in place.
 * Returns the number of ready entries, 0 on timeout, or SOCKET_ERROR.
 * A negative timeout waits indefinitely. The caller must inspect revents
 * for POLLERR/POLLHUP/POLLNVAL before consuming a readable socket.
 *
 * This is a Winsock building block, not a replacement for UDPST's
 * connection-index registration and console event handling.
 */
static int udpst_win32_poll_udp(WSAPOLLFD *entries, ULONG count, int timeout_ms)
{
    ULONG i;
    if (entries == NULL || count == 0)
        return SOCKET_ERROR;
    for (i = 0; i < count; ++i) {
        if (entries[i].fd == INVALID_SOCKET)
            return SOCKET_ERROR;
        entries[i].revents = 0;
    }
    return WSAPoll(entries, count, timeout_ms);
}

/*
 * Resolve a UDP destination for either IPv4 or IPv6.
 * Caller owns *result and must release it with freeaddrinfo().
 * No network traffic is sent by this operation.
 */
static int udpst_win32_resolve_udp(const char *hostname, const char *service,
                                   struct addrinfo **result)
{
    struct addrinfo hints;
    if (hostname == NULL || service == NULL || result == NULL)
        return EAI_FAIL;
    *result = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    return getaddrinfo(hostname, service, &hints, result);
}

#endif /* UDPST_WIN32_PLATFORM_H */
