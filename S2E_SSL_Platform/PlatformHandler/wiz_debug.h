/*
 * wiz_debug.h
 *
 *  Created on: Jan 19, 2021
 *      Author: Mason
 */

#ifndef PLATFORMHANDLER_WIZ_DEBUG_H_
#define PLATFORMHANDLER_WIZ_DEBUG_H_

#include "secure_nsc.h"

/** Define the debugging switch: on */
#define DBG_ON                  1
/** Define the debugging switch: off */
#define DBG_OFF                 0

#define DBG_LEVEL_INFO          DBG_ON
/** Define the debugging level: warning */
#define DBG_LEVEL_WARNING       DBG_ON
/** Define the debugging level: error */
#define DBG_LEVEL_ERR           DBG_ON
/** Define the debugging level: dump */
#define DBG_LEVEL_DUMP          DBG_ON
/** Define the debugging level: DHCP */
#define DBG_LEVEL_DHCP          DBG_ON
/** Define the debugging level: SEGCP */
#define DBG_LEVEL_SEGCP         DBG_ON
/** Define the debugging level: SEG */
#define DBG_LEVEL_SEG           DBG_ON
/** Define the debugging level: MQTT */
#define DBG_LEVEL_MQTT          DBG_ON
/** Define the debugging level: SSL */
#define DBG_LEVEL_SSL           DBG_ON
/** Define the debugging level: SSL */
#define DBG_LEVEL_SECURE        DBG_ON
/** general debug info switch, default: off */
#define GENERAL_DBG             DBG_ON

#define USE_SECURE_PRINTF		1

#if USE_SECURE_PRINTF
void WIZ_SECURE_printf(const char *format, ...);
#define SECURE_debug WIZ_SECURE_printf
#else
#define SECURE_debug printf
#endif


#define __DBGPRT_INFO(fmt, ...)				     \
do {									                     \
       SECURE_debug("[INFO] %s : "fmt,  __func__ , ##__VA_ARGS__); \
} while (0)

#define __DBGPRT_WARN(fmt, ...)				     \
do {									                     \
       SECURE_debug("[WARN] %s : "fmt,  __func__ , ##__VA_ARGS__); \
} while (0)


#define __DBGPRT_ERR(fmt, ...)				     \
do {									                     \
       SECURE_debug("[ERR] %s : "fmt,  __func__ , ##__VA_ARGS__); \
} while (0)

#define __DBGPRT_DHCP(fmt, ...)				     \
do {									                     \
       SECURE_debug("[DHCP] %s : "fmt,  __func__ , ##__VA_ARGS__); \
} while (0)

#define __DBGPRT_SEGCP(fmt, ...)				     \
do {									                     \
       SECURE_debug("[SEGCP] %s %d: "fmt,  __func__, __LINE__ , ##__VA_ARGS__); \
} while (0)

#define __DBGPRT_SEG(fmt, ...)				     \
do {									                     \
       SECURE_debug("[SEG] %s : "fmt,  __func__ , ##__VA_ARGS__); \
} while (0)

#define __DBGPRT_MQTT(fmt, ...)				     \
do {									                     \
       SECURE_debug("[MQTT] %s : "fmt,  __func__ , ##__VA_ARGS__); \
} while (0)

#define __DBGPRT_SSL(fmt, ...)				     \
do {									                     \
       SECURE_debug("[SSL] %s : "fmt,  __func__ , ##__VA_ARGS__); \
} while (0)

#define __DBGPRT_SECURE(fmt, ...)				     \
do {									                     \
       SECURE_debug("[SECURE] %s : "fmt,  __func__ , ##__VA_ARGS__); \
} while (0)


/**
 * @defgroup System_APIs System APIs
 * @brief System APIs
 */

/**
 * @addtogroup System_APIs
 * @{
 */

/**
 * @defgroup DEBUG_APIs DEBUG APIs
 * @brief DEBUG APIs
 */

/**
 * @addtogroup DEBUG_APIs
 * @{
 */

#if (GENERAL_DBG && DBG_LEVEL_INFO)
/** Print information of the info level */
#define PRT_INFO(f, a...)     __DBGPRT_INFO(f, ##a)
#else
/** Print information of the info level */
#define PRT_INFO(f, a...)
#endif

#if (GENERAL_DBG && DBG_LEVEL_WARNING)
/** Print information of the warning level */
#define PRT_WARN(f, a...)  __DBGPRT_WARN(f, ##a)
#else
/** Print information of the warning level */
#define PRT_WARN(f, a...)
#endif

#if (GENERAL_DBG && DBG_LEVEL_ERR)
/** Print information of the error level */
#define PRT_ERR(f, a...)      __DBGPRT_ERR(f, ##a)
#else
/** Print information of the error level */
#define PRT_ERR(f, a...)
#endif

#if (GENERAL_DBG && DBG_LEVEL_DHCP)
/** Print information of the error level */
#define PRT_DHCP(f, a...)      __DBGPRT_DHCP(f, ##a)
#else
/** Print information of the error level */
#define PRT_DHCP(f, a...)
#endif

#if (GENERAL_DBG && DBG_LEVEL_SEGCP)
/** Print information of the error level */
#define PRT_SEGCP(f, a...)      __DBGPRT_SEGCP(f, ##a)
#else
/** Print information of the error level */
#define PRT_SEGCP(f, a...)
#endif

#if (GENERAL_DBG && DBG_LEVEL_SEG)
/** Print information of the error level */
#define PRT_SEG(f, a...)      __DBGPRT_SEG(f, ##a)
#else
/** Print information of the error level */
#define PRT_SEG(f, a...)
#endif

#if (GENERAL_DBG && DBG_LEVEL_MQTT)
/** Print information of the error level */
#define PRT_MQTT(f, a...)      __DBGPRT_MQTT(f, ##a)
#else
/** Print information of the error level */
#define PRT_MQTT(f, a...)
#endif

#if (GENERAL_DBG && DBG_LEVEL_SSL)
/** Print information of the error level */
#define PRT_SSL(f, a...)      __DBGPRT_SSL(f, ##a)
#else
/** Print information of the error level */
#define PRT_SSL(f, a...)
#endif

#if (GENERAL_DBG && DBG_LEVEL_SECURE)
/** Print information of the error level */
#define PRT_SECURE(f, a...)      __DBGPRT_SECURE(f, ##a)
#else
/** Print information of the error level */
#define PRT_SECURE(f, a...)
#endif


#if (GENERAL_DBG && DBG_LEVEL_DUMP)
/**
 * @brief          dump memory
 *
 * @param[in]      *p     pointer the memory
 * @param[in]      len    length of memory
 *
 * @return         None
 *
 * @note           None
 */
void    PRT_DUMP(char *p, uint32_t len);
#else
/** Print information of the dump level */
#define PRT_DUMP(p, len)
#endif

#endif /* PLATFORMHANDLER_WIZ_DEBUG_H_ */
