#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <limits.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/sysinfo.h>

static int get_ip(void)
{
    struct ifaddrs *interfaces = NULL;

    printf("Adresse IPv4 locale :\n");
    if (getifaddrs(&interfaces) == -1) {
        perror("getifaddrs");
        return -1;
    }

    int found = 0;
    for (const struct ifaddrs *interface = interfaces;
         interface != NULL;
         interface = interface->ifa_next) {
        if (interface->ifa_addr == NULL ||
            interface->ifa_addr->sa_family != AF_INET ||
            (interface->ifa_flags & IFF_UP) == 0U ||
            (interface->ifa_flags & IFF_LOOPBACK) != 0U) {
            continue;
        }

        const struct sockaddr_in *address =
            (const struct sockaddr_in *)interface->ifa_addr;
        char address_text[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &address->sin_addr, address_text,
                      sizeof(address_text)) != NULL) {
            printf("%s\n", address_text);
            found = 1;
            break;
        }
    }

    freeifaddrs(interfaces);
    if (!found) {
        fprintf(stderr, "Aucune adresse IPv4 locale disponible.\n");
        return -1;
    }
    return 0;
}

static int read_cpu_counters(unsigned long long *total,
                             unsigned long long *idle)
{
    unsigned long long counters[8];
    FILE *file = fopen("/proc/stat", "r");
    if (file == NULL) {
        perror("fopen /proc/stat");
        return -1;
    }

    const int parsed = fscanf(file, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                              &counters[0], &counters[1], &counters[2],
                              &counters[3], &counters[4], &counters[5],
                              &counters[6], &counters[7]);
    if (fclose(file) != 0) {
        perror("fclose /proc/stat");
        return -1;
    }
    if (parsed != 8) {
        fprintf(stderr, "Impossible de lire les compteurs CPU dans /proc/stat.\n");
        return -1;
    }

    *total = 0;
    for (size_t index = 0; index < 8U; ++index) {
        if (ULLONG_MAX - *total < counters[index]) {
            fprintf(stderr, "Débordement des compteurs CPU.\n");
            return -1;
        }
        *total += counters[index];
    }

    if (ULLONG_MAX - counters[3] < counters[4]) {
        fprintf(stderr, "Débordement du compteur CPU idle.\n");
        return -1;
    }
    *idle = counters[3] + counters[4];
    return 0;
}

static int get_cpu_usage(void)
{
    unsigned long long total_before;
    unsigned long long idle_before;
    unsigned long long total_after;
    unsigned long long idle_after;

    if (read_cpu_counters(&total_before, &idle_before) != 0) {
        return -1;
    }

    const struct timespec interval = {.tv_sec = 1, .tv_nsec = 0};
    struct timespec remaining = interval;
    while (nanosleep(&remaining, &remaining) == -1) {
        if (errno != EINTR) {
            perror("nanosleep");
            return -1;
        }
    }

    if (read_cpu_counters(&total_after, &idle_after) != 0) {
        return -1;
    }
    if (total_after <= total_before || idle_after < idle_before) {
        fprintf(stderr, "Les compteurs CPU n'ont pas progressé comme prévu.\n");
        return -1;
    }

    const unsigned long long total_difference = total_after - total_before;
    const unsigned long long idle_difference = idle_after - idle_before;
    if (idle_difference > total_difference) {
        fprintf(stderr, "Compteurs CPU incohérents.\n");
        return -1;
    }

    const double usage =
        100.0 * (double)(total_difference - idle_difference) /
        (double)total_difference;
    printf("Utilisation CPU : %.2f%%\n", usage);
    return 0;
}

static int get_ram_usage(void)
{
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
        perror("sysinfo");
        return -1;
    }

    const uint64_t unit = (uint64_t)info.mem_unit;
    if (unit == 0U || (uint64_t)info.totalram > UINT64_MAX / unit ||
        (uint64_t)info.freeram > UINT64_MAX / unit) {
        fprintf(stderr, "Valeurs mémoire système invalides.\n");
        return -1;
    }

    const uint64_t total_bytes = (uint64_t)info.totalram * unit;
    const uint64_t free_bytes = (uint64_t)info.freeram * unit;
    if (free_bytes > total_bytes) {
        fprintf(stderr, "Valeurs mémoire système incohérentes.\n");
        return -1;
    }

    const uint64_t mebibyte = UINT64_C(1024) * UINT64_C(1024);
    const uint64_t used_ram = (total_bytes - free_bytes) / mebibyte;
    const uint64_t total_ram = total_bytes / mebibyte;
    printf("RAM utilisée : %" PRIu64 " Mo / %" PRIu64 " Mo\n",
           used_ram, total_ram);
    return 0;
}

int main(void)
{
    int status = EXIT_SUCCESS;
    printf("Informations système :\n\n");

    if (get_ip() != 0) {
        status = EXIT_FAILURE;
    }
    if (get_cpu_usage() != 0) {
        status = EXIT_FAILURE;
    }
    if (get_ram_usage() != 0) {
        status = EXIT_FAILURE;
    }

    return status;
}
