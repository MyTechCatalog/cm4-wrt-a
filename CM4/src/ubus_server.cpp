/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "ubus_server.hpp"
#include "Utils.hpp"
#include "pico_pkt_temperature.h"
#include "pico_pkt_fan_pwm.h"
#include "pico_pkt_watchdog.h"
#include "SensorID.hpp"
#include "version.h"
#include "TMP103_I2C.hpp"
#include "InfluxDB.hpp"

#define UBUS_OBJECT_TYPE_(_name, _methods) \
    {                                      \
        .name = _name,                     \
        .id = 0,                           \
        .methods = _methods,               \
        .n_methods = ARRAY_SIZE(_methods)  \
    }

#define	UBUS_EVENT_TEMPERATURE	"temperature_c"
#define	UBUS_EVENT_TACHOMETER	"tachometer_rpm"

namespace picod
{
    enum {        
        WATCHDOG_ENABLE,
        WATCHDOG_TIMEOUT,
        WATCHDOG_MAX_RETRIES,
        __WATCHDOG_MAX
    };

    enum {
        FAN_NAME,
        FAN_PWM_PERCENT,        
        __FAN_PWM_MAX
    };

    const struct blobmsg_policy watchdog_policy[] = {        
        [WATCHDOG_ENABLE] = {.name = "is_enabled", .type = BLOBMSG_TYPE_BOOL},
        [WATCHDOG_TIMEOUT] = {.name = "timeout_sec", .type = BLOBMSG_TYPE_INT32},
        [WATCHDOG_MAX_RETRIES] = {.name = "max_retries", .type = BLOBMSG_TYPE_INT32},
    };

    const struct blobmsg_policy fan_pwm_policy[] = {
        [FAN_NAME] = {.name = "fan_name", .type = BLOBMSG_TYPE_STRING},     
        [FAN_PWM_PERCENT] = {.name = "fan_pwm_pct", .type = BLOBMSG_TYPE_INT32}
    };

    const struct blobmsg_policy version_policy[] = {};
    const struct blobmsg_policy status_policy[] = {};

    struct ubus_context *g_ctx;
    int notify = 0;

    const struct ubus_method picod_methods[] = {
        UBUS_METHOD("version", picod_version, version_policy),
        UBUS_METHOD("status", picod_status, status_policy),
        UBUS_METHOD("watchdog", picod_watchdog, watchdog_policy),
        UBUS_METHOD("fan_pwm", picod_fan_pwm, fan_pwm_policy)
        };

    struct ubus_object_type picod_object_type =
        UBUS_OBJECT_TYPE_("picod", picod_methods);

    struct ubus_object picod_object = {
        .name = "picod",
        .type = &picod_object_type,
        .subscribe_cb = picod_subscribe_cb,
        .methods = picod_methods,
        .n_methods = ARRAY_SIZE(picod_methods),
    };

    struct uloop_timeout notify_temperature_timer = {
        .cb = picod_notify_temperature_cb,
    };

    struct blob_buf b;
    struct blob_buf watchdog_blob;
    struct blob_buf temperature_blob;
    struct blob_buf tachometer_blob;
    struct blob_buf pwm_blob;
    std::string picoVersion;

    void put_container(struct blob_buf *buf, struct blob_attr *attr, const char *name) {
         void *c = blobmsg_open_table(buf, name);
         blob_put_raw(buf, blob_data(attr), blob_len(attr));
         blobmsg_close_table(buf, c);
    }

    void picod_subscribe_cb(struct ubus_context *ctx, struct ubus_object *obj) {
        notify = obj->has_subscribers;
        //fprintf(stderr, "Subscribers active: %d\n", obj->has_subscribers);
    }

    void picod_bcast_event(char *event, struct blob_attr *msg) {
        int ret;

        if (!notify)
            return;

        ret = ubus_notify(g_ctx, &picod_object, event, msg, -1);
        if (ret)
            fprintf(stderr, "Failed to notify log: %s\n", ubus_strerror(ret));
    }

    int picod_version(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method, struct blob_attr *msg) {
        blob_buf_init(&b, 0);

        if (!picoVersion.empty()) {
            blobmsg_add_string(&b, "pico_version", picoVersion.c_str());
        }

        blobmsg_add_string(&b, "picod_version", VERSION_STR);

        ubus_send_reply(ctx, req, b.head);

        return UBUS_STATUS_OK;
    }

    int picod_status(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method, struct blob_attr *msg) {

        blob_buf_init(&b, 0);
        
        bool rw_flag = false; // Read(0)/Write(1) flag        
        std::vector<struct pico_pkt_fan_pwm_t> fanInfo;
        fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
            .fan_id = SYS_FAN1, .pwm_pct = 0.0f });
        fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
            .fan_id = CM4_FAN, .pwm_pct = 0.0f });

        if (send_fan_pwm_request(rw_flag, fanInfo)) {
            blob_buf_init(&pwm_blob, 0);
            
            blobmsg_add_u32(&pwm_blob,
                appSettings.sensorIds[picod::System_FAN_J17].c_str(), (int)(fanInfo[SYS_FAN1-1].pwm_pct*100));
            
            blobmsg_add_u32(&pwm_blob,
                appSettings.sensorIds[picod::CM4_FAN_J18].c_str(), (int)(fanInfo[CM4_FAN-1].pwm_pct*100));
            
            put_container(&b, pwm_blob.head, fan_pwm_policy[FAN_PWM_PERCENT].name);           
        }
        
        pico_pkt_watchdog_t s = {0};
        s.write = false; 
        if (send_watchdog_request(s)) {
            blob_buf_init(&watchdog_blob, 0);                     
            blobmsg_add_string(&watchdog_blob, watchdog_policy[WATCHDOG_ENABLE].name, s.enable ? "true" : "false");
            blobmsg_add_u16(&watchdog_blob, watchdog_policy[WATCHDOG_TIMEOUT].name, s.timeout);
            blobmsg_add_u16(&watchdog_blob, watchdog_policy[WATCHDOG_MAX_RETRIES].name, s.max_retries);
            put_container(&b, watchdog_blob.head, "watchdog");
        }

        pico_pkt_temperature_u t = {0};
        if (send_temperature_request(t)) {
            blob_buf_init(&temperature_blob, 0);
            blob_buf_init(&tachometer_blob, 0);
            
            if (appSettings.sensorIds.size() > NUM_NTC_SENSORS) {
                for (int ch=0; ch < NUM_NTC_SENSORS; ch++) {
                    blobmsg_add_string(&temperature_blob, 
                        appSettings.sensorIds[ch].c_str(), 
                        formatDouble(t.data[ch]).c_str());
                }
            }

            if (picod::SensorId::NUM_SENSOR_IDs == appSettings.sensorIds.size()) {                
                blobmsg_add_string(&temperature_blob, 
                        appSettings.sensorIds[picod::RPi_Pico].c_str(), 
                        formatDouble(t.s.pico).c_str());
                
                if (appSettings.enable_tmp103_sensor) {            
                    blobmsg_add_string(&temperature_blob, 
                        appSettings.sensorIds[picod::Under_CM4_SOC].c_str(), 
                        formatDouble(TMP103_I2C::instance().getTemperature(), 0).c_str());
                }

                blobmsg_add_u32(&tachometer_blob,
                    appSettings.sensorIds[picod::System_FAN_J17].c_str(), t.s.fan1rpm);
                blobmsg_add_u32(&tachometer_blob,
                    appSettings.sensorIds[picod::CM4_FAN_J18].c_str(), t.s.cm4_fan_rpm);
            }
            put_container(&b, temperature_blob.head, UBUS_EVENT_TEMPERATURE);
            put_container(&b, tachometer_blob.head, UBUS_EVENT_TACHOMETER);
        }
        
        ubus_send_reply(ctx, req, b.head);

        return UBUS_STATUS_OK;
    }

    void picod_notify_temperature_cb(struct uloop_timeout *timeout){

        pico_pkt_temperature_u t = {0};
        
        if (send_temperature_request(t)) {
            blob_buf_init(&temperature_blob, 0);
            blob_buf_init(&tachometer_blob, 0);
            
            if (appSettings.sensorIds.size() > NUM_NTC_SENSORS) {
                for (int ch=0; ch < NUM_NTC_SENSORS; ch++) {
                    blobmsg_add_string(&temperature_blob, 
                        appSettings.sensorIds[ch].c_str(), 
                        formatDouble(t.data[ch]).c_str());

                    if (appSettings.enable_influx_db) {
                        picod::InfluxDB::instance().addTemperature(
                            appSettings.sensorIds[ch], t.data[ch]);
                    }
                }
            }

            if (picod::SensorId::NUM_SENSOR_IDs == appSettings.sensorIds.size()) {                
                blobmsg_add_string(&temperature_blob, 
                        appSettings.sensorIds[picod::RPi_Pico].c_str(), 
                        formatDouble(t.s.pico).c_str());

                if (appSettings.enable_influx_db) {
                    picod::InfluxDB::instance().addTemperature(
                        appSettings.sensorIds[picod::RPi_Pico], t.s.pico);

                    picod::InfluxDB::instance().addTachometer(
                        appSettings.sensorIds[picod::System_FAN_J17], t.s.fan1rpm);
                }
                
                if (appSettings.enable_tmp103_sensor) {
                    int retVal = TMP103_I2C::instance().getTemperature(); 
                    
                    blobmsg_add_string(&temperature_blob, 
                        appSettings.sensorIds[picod::Under_CM4_SOC].c_str(), 
                        formatDouble(retVal, 0).c_str());

                    if (appSettings.enable_influx_db) {
                        picod::InfluxDB::instance().addTemperature(
                        appSettings.sensorIds[picod::Under_CM4_SOC], retVal);
                    }
                }

                blobmsg_add_u32(&tachometer_blob,
                    appSettings.sensorIds[picod::System_FAN_J17].c_str(), t.s.fan1rpm);
                
                blobmsg_add_u32(&tachometer_blob,
                    appSettings.sensorIds[picod::CM4_FAN_J18].c_str(), t.s.cm4_fan_rpm);

                if (appSettings.enable_influx_db) {
                    picod::InfluxDB::instance().publish();
                }              
            }

            picod_bcast_event((char*)UBUS_EVENT_TEMPERATURE, temperature_blob.head);
            picod_bcast_event((char*)UBUS_EVENT_TACHOMETER, tachometer_blob.head);            
        }
        
        uloop_timeout_set(&notify_temperature_timer, 
            static_cast<int>(appSettings.temperature_poll_interval_seconds * 1000));
    }

    int picod_watchdog(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method, struct blob_attr *msg) {
        
        struct blob_attr *tb[__WATCHDOG_MAX];

        blobmsg_parse(watchdog_policy, __WATCHDOG_MAX, tb, blob_data(msg), blob_len(msg));
        if (!tb[WATCHDOG_ENABLE] || !tb[WATCHDOG_TIMEOUT] || !tb[WATCHDOG_MAX_RETRIES]){
            return UBUS_STATUS_INVALID_ARGUMENT;
        }            
        
        pico_pkt_watchdog_t s = {0};
        s.enable = blobmsg_get_bool(tb[WATCHDOG_ENABLE]);
        s.timeout = blobmsg_get_u32(tb[WATCHDOG_TIMEOUT]);
        s.max_retries = blobmsg_get_u32(tb[WATCHDOG_MAX_RETRIES]);
        s.timeout = sanitize_watchdog_timeout(s.timeout);        
        s.write = true;
        if (send_watchdog_request(s)) {
            blob_buf_init(&b, 0);
            blobmsg_add_string(&b, blobmsg_name(tb[WATCHDOG_ENABLE]), s.enable ? "true" : "false");
            blobmsg_add_u16(&b, blobmsg_name(tb[WATCHDOG_TIMEOUT]), s.timeout);
            blobmsg_add_u16(&b, blobmsg_name(tb[WATCHDOG_MAX_RETRIES]), s.max_retries);
            ubus_send_reply(ctx, req, b.head);
        } else {
            return UBUS_STATUS_UNKNOWN_ERROR;
        }
        
        return UBUS_STATUS_OK;
    }

    int picod_fan_pwm(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method, struct blob_attr *msg) {
        
        struct blob_attr *tb[__FAN_PWM_MAX];

        blobmsg_parse(fan_pwm_policy, __FAN_PWM_MAX, tb, blob_data(msg), blob_len(msg));
        if (!tb[FAN_NAME]) {
            return UBUS_STATUS_INVALID_ARGUMENT;
        }
        
        char * fan_name = blobmsg_get_string(tb[FAN_NAME]);
        uint32_t fan_pwm_pct = 0;
        bool rw_flag = false;

        if (tb[FAN_PWM_PERCENT]) {
            rw_flag = true;
            fan_pwm_pct = blobmsg_get_u32(tb[FAN_PWM_PERCENT]);
        }
        
        fan_pwm_pct = fan_pwm_pct > 100 ? 100 : fan_pwm_pct;
        float fan_pwm =  static_cast<float>(fan_pwm_pct)/FAN_PWM_LSB;
        uint8_t fan_id = get_fan_id_from_name(fan_name);

        if (fan_id ==  INVALID_FAN_ID) {
            return UBUS_STATUS_INVALID_ARGUMENT;
        }        
        
        std::vector<struct pico_pkt_fan_pwm_t> fanInfo;
        fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
                .fan_id = static_cast<uint8_t>(fan_id), .pwm_pct = fan_pwm });

        if (send_fan_pwm_request(rw_flag, fanInfo)) {
            blob_buf_init(&b, 0);
            blobmsg_add_u32(&b, fan_pwm_policy[FAN_PWM_PERCENT].name, (int)(fanInfo[0].pwm_pct*100));
            ubus_send_reply(ctx, req, b.head);
        } else {
            return UBUS_STATUS_UNKNOWN_ERROR;
        }

        return UBUS_STATUS_OK;
    }

    void server_main(struct ubus_context *ctx) {
        int ret;

        ret = ubus_add_object(ctx, &picod_object);
        if (ret)
            fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));

        uloop_timeout_set(&notify_temperature_timer, 
            static_cast<int>(appSettings.temperature_poll_interval_seconds * 1000));

        uloop_run();
    }

    int run_ubus_server(const char *ubus_socket) {
        uloop_init();
        signal(SIGPIPE, SIG_IGN);

        g_ctx = ubus_connect(ubus_socket);
        if (!g_ctx) {
            fprintf(stderr, "Failed to connect to ubus\n");
            return -1;
        }

        ubus_add_uloop(g_ctx);

        server_main(g_ctx);

        ubus_free(g_ctx);
        uloop_done();

        return 0;
    }

} //@END namespace picod