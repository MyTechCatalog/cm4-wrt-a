/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef UBUS_SERVER_HPP
#define UBUS_SERVER_HPP
#include <unistd.h>
#include <signal.h>
#include "libubus.h"
#include <string>

namespace picod {
    extern int pico_fd;
    extern std::string picoVersion;

    void picod_subscribe_cb(struct ubus_context *ctx, struct ubus_object *obj);

    void picod_notify_temperature_cb(struct uloop_timeout *timeout);
    
    int picod_version(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg);

    int picod_status(struct ubus_context *ctx, struct ubus_object *obj,
            struct ubus_request_data *req, const char *method, struct blob_attr *msg);

    int picod_watchdog(struct ubus_context *ctx, struct ubus_object *obj,
            struct ubus_request_data *req, const char *method, struct blob_attr *msg);
    
    int picod_fan_pwm(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method, struct blob_attr *msg);

    void server_main(void);
    int run_ubus_server(const char *ubus_socket = nullptr);

}//@END namespace picod

#endif 