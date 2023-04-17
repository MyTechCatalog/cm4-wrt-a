#include <stdio.h>
#include <cerrno>
#include <cstring> // memset
#include <unistd.h>
#include <iostream>
#include "Utils.hpp"
#include "pico_pkt_ping.h"

void ping_the_pico(int fd) {
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    pico_pkt_ping_req_pack((uint8_t *)pkt.req);
    
    print_bytes("Request data:", pkt.req, PICO_PKT_LEN);

    send_serial_port_command(fd, pkt.req, PICO_PKT_LEN);
}

void pkt_ping(struct pkt_buf *b)
{   
    bool success = true;
    pico_pkt_ping_resp_unpack((uint8_t *)b->resp, &success);
    print_bytes("Ping reponse data:", b->resp, PICO_PKT_LEN);
    
}

void pkt_simulate_pico_ping_reponse(struct pkt_buf *b)
{   
    const bool success = true;
    for (size_t i = PICO_PKT_PING_RESV; i < PICO_PKT_LEN; i++) {
        b->resp[i] = b->req[i];
    }
    pico_pkt_ping_resp_pack((uint8_t *)b->resp, success);
}
