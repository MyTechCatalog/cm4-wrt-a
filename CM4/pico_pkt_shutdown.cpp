/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "Utils.hpp"
#include "pico_pkt_shutdown.h"

void pkt_shutdown(struct pkt_buf *b){
    //print_bytes("Shutdown Request:", b->resp, PICO_PKT_LEN);
    run_cmd((char *)"poweroff");
}
