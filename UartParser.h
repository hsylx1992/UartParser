#ifndef _UARTPARSER_H_
#define _UARTPARSER_H_


typedef UINT32 led_bitmap_t;

typedef enum enUartId__t {
    ID_RX_START = 0,    
    ID_RX_MB_ADDR,
    ID_RX_WP_ID,
    ID_RX_WP_ADDR,
    ID_RX_WC_ADDR,//wxr
    ID_RX_END,
    
    ID_TX_START,
    ID_TX_MB_ADDR,
    ID_TX_WP_ID,
    ID_TX_WP_ADDR,
    ID_TX_WC_ADDR,//wxr
    ID_TX_END,
    
    ID_RTX_END
} enUartId_t;


#endif