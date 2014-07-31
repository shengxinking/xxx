#ifndef _COMMON_H
#define _COMMON_H

#define S_LEN 	32	
#define M_LEN 	128
#define L_LEN  	512

#define RULE_DB    "signature.db"
#define CONFIG_DB  "_config"

#define MAX_TXT_LEN  512 
#define MAX_TXT_LEN2 32 

#define RET_OK          1
#define RET_FAIL       -1
#define RET_PASS        2

#define MD_NONE         0
#define MD_RULE_CHECK   1
#define MD_COUNT        2



//#define DBG_RULE 1      
#ifdef  DBG_RULE                  
#define DBGR(fmt, args...)                                      \
        do {                                                            \
                printf("[%s][%d] : \n\t"fmt,            \
                                __FUNCTION__,__LINE__, ##args);                    \
        } while (0)
#else
#define DBGR(fmt, args...)
#endif


#define DBG_ATTACK 1      
#ifdef  DBG_ATTACK                  
#define DBGA(fmt, args...)                                      \
        do {                                                            \
                printf("[%s][%d] : \n\t"fmt,            \
                                __FUNCTION__,__LINE__, ##args);                    \
        } while (0)
#else
#define DBGA(fmt, args...)
#endif

//#define DBG_KEY 1      
#ifdef  DBG_KEY                  
#define DBGK(fmt, args...)                                      \
        do {                                                            \
                printf("[%s][%d] : \n\t"fmt,            \
                                __FUNCTION__,__LINE__, ##args);                    \
        } while (0)
#else
#define DBGK(fmt, args...)
#endif


#define DBG_CFG 1      
#ifdef  DBG_CFG                  
#define DBGC(fmt, args...)                                      \
        do {                                                            \
                printf("[%s][%d] : \n\t"fmt,            \
                                __FUNCTION__,__LINE__, ##args);                    \
        } while (0)
#else
#define DBGC(fmt, args...)
#endif

//#define DBG_HTTP 1      
#ifdef  DBG_HTTP                  
#define DBGH(fmt, args...)                                      \
        do {                                                            \
                printf("[%s][%d] : \n\t"fmt,            \
                                __FUNCTION__,__LINE__, ##args);                    \
        } while (0)
#else
#define DBGH(fmt, args...)
#endif

#define DBG_NET 1      
#ifdef  DBG_NET                  
#define DBGN(fmt, args...)                                      \
        do {                                                            \
                printf("[%s][%d] : \n\t"fmt,            \
                                __FUNCTION__,__LINE__, ##args);                    \
        } while (0)
#else
#define DBGN(fmt, args...)
#endif

#define DBG_LOG 1      
#ifdef  DBG_LOG                  
#define DBGL(fmt, args...)                                      \
        do {                                                            \
                printf("[%s][%d] : \n\t"fmt,            \
                                __FUNCTION__,__LINE__, ##args);                    \
        } while (0)
#else
#define DBGL(fmt, args...)
#endif

#define DBG_CFG_FILE 1      
#ifdef  DBG_CFG_FILE                  
#define DBGF(fmt, args...)                                      \
        do {                                                            \
                printf("[%s][%d] : \n\t"fmt,            \
                                __FUNCTION__,__LINE__, ##args);                    \
        } while (0)
#else
#define DBGF(fmt, args...)
#endif


#endif

