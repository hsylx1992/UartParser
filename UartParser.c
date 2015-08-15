
#include "UartParser.h"
#include "halUart.h"
#include "halTimer.h"

//=========global variables=============================


//=============local variable========================
static UINT32 WiapaId = 0;
static UINT32 WiapaAddr_16;
static BYTE   WiapaAddr_a2[2];
static BYTE   BuffForWatch[UARTPARSER_BUFSIZE] = {0};

static UINT8  aUartMsgLen[] = {
    0,
    19,
    11,
    13,
    20,//wxr
    0,
    0,
    18,
    10,
    12,
    19,//wxr
    0,
    0
};

static char  *szUartMsg[] = {
    "NeverHere_ID_RX_START",
    "SendModbusSlaveAddr",
    "SendWiapaID",
    "SendWiapaAddr", 
    "SendWiapaChannelAddr",//wxr 
    "NeverHere_ID_RX_END",

    "NeverHere_ID_TX_START",
    "GetModbusSlaveAddr",
    "GetWiapaID",
    "GetWiapaAddr", 
    "GetWiapaChannelAddr",//wxr
    "NeverHere_ID_TX_END",
    
    "NerverHere_ID_RTX_END"
};

//============local variable declaration===============
static UINT8 UartGetMsg(BYTE a[UARTPARSER_BUFSIZE]);
static UINT8 DealVarietyMsg(BYTE a[UARTPARSER_BUFSIZE], UINT8 len, UINT8 *offset[ID_RTX_END]);

//============global methods============================
UINT8 UartMsgProcess() {
    BYTE  a[UARTPARSER_BUFSIZE] = {0};   
    UINT8 len = 0;
    
    memset(a, 0, sizeof(a));
    len = UartGetMsg(a);
    
    if (!len) {
        //没有收到数据
        return -1;
    }
    
    //收到数据
    memcpy(led_BuffForWatch, a, sizeof(a));
    bitmap_t map = 0x00;
    UINT8 *offset[ID_RTX_END] = {NULL}; 
    
    UINT8 type = DealVarietyMsg(a, len, offset);
    
    return 0;
}

//=============local methods=========================
static
UINT8 parserError(UINT8 *src) {
    return -1;
}

static
UINT8 parserSendModbusSlaveAddr(UINT8 *src) {
    return 0;
}

static
UINT8 parserSendWiapaID(UINT8 *src) {
    
    WiapaId = src[0]; 
 
    return 0;
}

static
UINT8 parserSendWiapaAddr(UINT8 *src) {

    WiapaAddr_a2[0] = src[0];   //高位
    WiapaAddr_a2[1] = src[1];   //低位

    WiapaAddr_16    = (led_WiapaAddr_a2[0] << 8) + WiapaAddr_a2[1];
      
    return 0;
}

static
UINT8 parserSendWiapaChannelAddr(UINT8 *src){
    return 0;
}

static
UINT8 parserGetModbusSlaveAddr(UINT8 *src) {
    return 0;
}

static
UINT8 parserGetWiapaID(UINT8 *src) {
    return 0;
}

static
UINT8 parserGetWiapaAddr(UINT8 *src) {
    return 0;
}

static
UINT8 parserGetWiapaChannelAddr(UINT8 *src) {  
    return 0;
}

//转移表
static UINT8 (*parserTable[])(UINT8 *src) = {
    parserError,
    parserSendModbusSlaveAddr,
    parserSendWiapaID,
    parserSendWiapaAddr,
    parserSendWiapaChannelAddr,
    parserError,
  
    parserError,
    parserGetModbusSlaveAddr,
    parserGetWiapaID,
    parserGetWiapaAddr,
    parserGetWiapaChannelAddr,
    parserError,
    
    parserError
};

static
UINT8 UartGetMsg(BYTE a[UARTPARSER_BUFSIZE]) {
    UINT32 timer = 0;
    UINT8  len = 0;
    memset(a, 0, sizeof(a));
    do {
        if (halGetchRdy()) {
            a[len++] = halGetch();
            timer= halGetMACTimer();
        }
    } while (halMACTimerNowDelta(timer)<MSECS_TO_MACTICKS(1));
    
    return len;
}

static
UINT8 AsciiTypeParser(BYTE a[UARTPARSER_BUFSIZE], UINT8 len) {
    UINT8 ret = 1;
    
    //在这里处理
    switch (a[0]) {
    case 0x06:
        //Opr1()
        break;
    case 0x03:
        //Opr2();
        break;
    case 0x41:
        //Opr3();
        break;
    default:
        //
        ret = 0;
        break;
    }
    if (ret) {
        // DoSomeCommonOpr();
    }
    
    return ret;
}

//返回：来自Uart的数据包含哪种类型的消息
//对应的有效载荷指针保存在offset中
static
led_bitmap_t StringTypeGetBitmap(BYTE a[UARTPARSER_BUFSIZE], UINT8 *offset[ID_RTX_END]) {
    bitmap_t map = 0x00;
    BYTE *p = NULL;
    
    for (int i = ID_RX_START+1; i < ID_RX_END; i++) {
        if(p = strstr(a, szUartMsg[i])) {
            map = map | (0x01<<(i));
            offset[i] = p+aUartMsgLen[i];
        }
    }
    for (int i = ID_TX_START+1; i < ID_TX_END; i++) {
        if(p = strstr(a, szUartMsg[i])) {
            map = map | (0x01<<(i));
            offset[i] = p+aUartMsgLen[i];
        }
    }
    return map;
}

static
UINT8 StringTypeParser(led_bitmap_t map, UINT8 *offset[ID_RTX_END]){
    UINT8 map_size = GetBitMapSize();
    
    for (UINT8 i = 0; i < map_size; i++) {
        if ((map >> i)&0x01) {
            parserTable[i](offset[i]);
        }
    }
}

static
UINT8 DealVarietyMsg(BYTE a[UARTPARSER_BUFSIZE], UINT8 len, UINT8 *offset[ID_RTX_END]) {
    
    bitmap_t map = StringTypeGetBitmap(a, offset);
    if (map) {
        StringTypeParser(map, offset);
        return 1;
    }
    
    //!map
    UINT8 ret = AsciiTypeParserMsg(a, len);
    if (ret) {
        return 2;
    }
    
    // !ret
    // TODO: more type
    
    
    // No available type
    return 0;
}

static
UINT8 GetBitMapSize() {
    return sizeof(led_bitmap_t)/sizeof(UINT8)*8;
}