/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef CLI_H_
#define CLI_H_

#include <string>

#define GET_BOOLEAN_SETTING(name, dataOut)\
{\
    int value = 0;\
    if (!config_lookup_bool(&config, name, &value)) {\
        fmt::println(stderr, "Failed to load setting "#name" from: {}", file_path);\
    } else {\
        dataOut = !!value;\
    }\
}

#define GET_FLOAT_SETTING(name, dataOut)\
{\
    double value = 0.0;\
    if (!config_lookup_float(&config, name, &value)) {\
        fmt::println(stderr, "Failed to load setting "#name" from: {}", file_path);\
    } else {\
        dataOut = value;\
    }\
}

#define GET_INTEGER32_SETTING(name, dataOut)\
{\
    int32_t value = 0;\
    if (!config_lookup_int(&config, name, &value)) {\
        fmt::println(stderr, "Failed to load setting "#name" from: {}", file_path);\
    } else {\
        dataOut = value;\
    }\
}

#define GET_INTEGER16_SETTING(name, dataOut)\
{\
    int32_t value = 0;\
    if (!config_lookup_int(&config, name, &value)) {\
        fmt::println(stderr, "Failed to load setting "#name" from: {}", file_path);\
    } else {\
        dataOut = value;\
    }\
}

#define GET_STRING_SETTING(name, dataOut)\
{\
    const char * temp = NULL;\
    if (!config_lookup_string(&config, name, &temp)) {\
        fmt::println(stderr, "Error loading "#name": {}. {}",\
            config_error_text(&config), config_error_file(&config));\
        fmt::println(stderr, "Failed to load setting "#name" from: {}", file_path);\
    } else {\
        dataOut = temp;\
    }\
}

void usage(char *app);
void parse_cli(int argc, char** argv);
int parse_config_file(const char * file_path);

#endif //CLI_H_