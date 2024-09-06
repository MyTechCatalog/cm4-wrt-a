/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "pkt_handler.h"
#include "pico_pkt_version.h"
#include <string>
#include <iostream>
#include <cstring> // memset
#include <unistd.h>
#include "Utils.hpp"
#include "PacketHandler.hpp"

void pkt_version(struct pkt_buf *b) {    
    static std::string picoVersion;
    bool pico_version_sop = false;   // Start of packet
    bool pico_version_eop = false;   // End of packet

    //print_bytes("Pico version data:", b->resp, PICO_PKT_LEN); 
    char *dataOut = pico_pkt_version_resp_unpack((uint8_t *)b->resp, 
        &pico_version_sop, &pico_version_eop);

    if (pico_version_sop) {
        picoVersion.clear();
        //std::cout << "Pico version Start of packet." << std::endl;
    }
    
    if (dataOut) {
        picoVersion.append(dataOut);
        free(dataOut);
        //std::cout << "Pico version chunk: " << picoVersion << std::endl;
    }

    if (pico_version_eop) {
        std::cout << "Pico Version: " << picoVersion << std::endl;
    }
}

void send_pico_version_read_request(int fd) {
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    // Pack the Pico build version request message
    pico_pkt_version_req_pack((uint8_t *)pkt.req);

    PacketHandler::instance().send_pico_request(pkt.req, PICO_PKT_LEN);
}

void get_pico_version(std::string & picoVersion) {
    bool pico_version_sop = false;   // Start of packet
    bool pico_version_eop = false;   // End of packet

    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    // Pack the Pico build version request message
    pico_pkt_version_req_pack((uint8_t *)pkt.req);

    PacketHandler::instance().send_pico_request(pkt.req, PICO_PKT_LEN);

    bool success = PacketHandler::instance().get_pico_response(pkt, PICO_PKT_VERSION_MAGIC);
    
    while ( success ) {

        char *dataOut = pico_pkt_version_resp_unpack((uint8_t *)pkt.resp, 
            &pico_version_sop, &pico_version_eop);

        if (pico_version_sop) {
            picoVersion.clear();
        }
        
        if (dataOut) {
            picoVersion.append(dataOut);
            free(dataOut);
        }

        if (pico_version_eop) {
            break;
        }
        

        memset((void *)&pkt.req, 0, sizeof(pkt.req));
        memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

        // Pack the Pico build version request message
        pico_pkt_version_req_pack((uint8_t *)pkt.req);

        PacketHandler::instance().send_pico_request(pkt.req, PICO_PKT_LEN);

        success = PacketHandler::instance().get_pico_response(pkt, PICO_PKT_VERSION_MAGIC);
    }
}//@END get_pico_version()