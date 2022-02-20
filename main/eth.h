#ifndef ETH_H
#define ETH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool eth_wifi_connect(const char *ap_ssid, const char *ap_pass);

bool eth_udp_init(int *sock);

bool eth_udp_tx(int         sock,
                void *      data,
                size_t      size,
                const char *dest_ipv4_addr,
                uint16_t    dest_port);

bool eth_udp_destroy(int sock);

#endif // ETH_H
