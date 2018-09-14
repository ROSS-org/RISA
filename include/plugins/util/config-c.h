#ifndef CONFIG_C_H
#define CONFIG_C_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

// this header describes any wrapper functions for C++ calls 
// needed by the C side of the ROSS-Damaris interface

/**
 * @brief parse the config file used for instrumentation and damaris
 */
EXTERNC void parse_file(const char *filename);

#undef EXTERNC

#endif // CONFIG_H
