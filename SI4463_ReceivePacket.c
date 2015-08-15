unsigned char SI4463_ReceivePacket(unsigned char *abPayload)
{
    unsigned char k;
    // Read ITs, clear pending ones
    //Si4463_GetIntStatus(0,0,0,SI446x_Status); 
    // Read ITs, do not clear pending ones
      
    Si4463_GetIntStatus(0xFB,0x7F,0x7F,SI446x_Status); 
    //CRCУ��
    if((SI446x_Status[2] & 0x08) == 0x08)
    {
        Si4463_GetIntStatus(0,0,0,SI446x_Status); 
        SI4463_StartRx(RX_DATA_LENGTH);
    }
    else if((SI446x_Status[2] & 0x10) == 0x10)	  ////����  Packet received    
    {
        // LED_Task();
        // Get RSSI
        SI4463_GetFastResponseReg(CMD_FAST_RESPONSE_REG_C,1,&bRssi);
        SI4463_ReadRxTxDataBuffer(RX_DATA_LENGTH, SI4463_ReadBuf);      
            
        //BYTE flen = 0x00;
        BYTE *ptr=NULL;
        BYTE *rx_frame=NULL;
        BYTE ack_bytes[5];
        BYTE crc;
        BYTE i;
        BYTE rxBytes;
#ifdef LRWPAN_MANUAL_MAC_ACK
        UINT8 seq;
#endif
        //define alternate names for readability in this function
        #define  fcflsb  ack_bytes[0]
        #define  fcfmsb  ack_bytes[1]
        #define  dstmode ack_bytes[2]
        #define  srcmode ack_bytes[3]
        rxBytes = SI4463_ReadBuf[0];

        /* mac list is full - do nothing, skip to end */
        if (macRxBuffFull())            { goto do_rxflush; }
        
        /* receive FIFO is empty - do nothing, skip to end */
        if (rxBytes == 0)               { goto do_rxflush; }
        
        /* receive FIFO is not empty, continue processing */
        
        /* ���Դ��� */
        /*
        rx_frame = MemAlloc(rxBytes + 3);
        ptr      = rx_frame;
        if(rx_frame==NULL)              { return 0; }
        
        *ptr++   = rxBytes;
        for(i=0;i<rxBytes;i++) {
            *ptr++ = SI4463_ReadBuf[i+1];
        }
        */
        //++++++++++++++++++++++++++++++++++
        #ifdef LED_COOR
        {
            BYTE puo_i = 0;
            for (puo_i; puo_i <rxBytes; puo_i++) {
              conPrintUINT8_noleader(SI4463_ReadBuf[puo_i+1]);
            }
            goto do_rxflush;
        }
        #endif
        //----------------------------------
        //++++++++++++++++++++++++++++++++++
        #ifdef PHYPKT_UART_OUT
        {
            BYTE puo_i = 0;
            conPrintROMString("Recv:\t");
            for (puo_i; puo_i <rxBytes; puo_i++) {
              conPrintUINT8_noleader(SI4463_ReadBuf[puo_i+1]);
            }
            conPrintROMString("\n");
            goto do_rxflush;
        }
        #endif
        //----------------------------------
                      
        /* read the first byte from FIFO - the packet length */
        // ע��: ����IEEE 15.4��ʽ�����ݰ��ĵ�һ���ֽ�����ʾ�ĳ��Ȱ�����CRC����
        // mrfiSpiReadRxFifo(&flen, MRFI_LENGTH_FIELD_SIZE);

        
        // ����CC1101��һ�������ֽڲ�����CRC�򳤶ȣ���Э��ջ�ϲ㴦��ʱ������IEEE 802.15.4ģʽ�������ֽ�
        // �������س��Ⱥ�CRC���ȡ�Ϊ�˱��ִ����ͳһ�ԣ���flen��Ϊ����CRC���ȵ�ֵ��
        
        //get the sequence number of the frame
        #ifdef LRWPAN_MANUAL_MAC_ACK                
        seq = SI4463_ReadBuf[3];
        #endif
        
        if (flen == LRWPAN_ACKFRAME_LENGTH) {
            //this should be an ACK.
            //read the packet, do not allocate space for it
            ack_bytes[0] = flen;
            for(i=0; i<rxBytes+1 ; i++) {
                ack_bytes[i]=SI4463_ReadBuf[i];
            }
            //crc = ack_bytes[LRWPAN_ACKFRAME_LENGTH-1];
            //check CRC
            if ( LRWPAN_IS_ACK(ack_bytes[1]))
            {
                // CRC ok, perform callback if this is an ACK
                // Convert the raw RSSI value and do offset compensation for this radio 
                ack_bytes[4] = bRssi;
                macRxCallback(ack_bytes, ack_bytes[4]);
            }
        } else {
            //not an ack packet, lets do some more early rejection
            //read the fcflsb, fcfmsb
            fcflsb = SI4463_ReadBuf[1];
            fcfmsb = SI4463_ReadBuf[2]; 
            srcmode = LRWPAN_GET_SRC_ADDR(fcfmsb);
            dstmode = LRWPAN_GET_DST_ADDR(fcfmsb);
            if ((srcmode == LRWPAN_ADDRMODE_NOADDR) && 
                (dstmode == LRWPAN_ADDRMODE_NOADDR)) {
                //reject this packet, no addressing info
                goto do_rxflush;
            }
            
            #define CRCBAD() 0
            if (CRCBAD()) {
                goto do_rxflush;
            }

            //CRC good
            //change the RSSI byte from 2's complement to unsigned number
            
            #ifdef LRWPAN_MANUAL_MAC_ACK
            if (LRWPAN_GET_ACK_REQUEST(fcflsb)) 
            {
                // only data frame and MAC command frame may need ACK.
                if ((LRWPAN_IS_DATA(fcflsb)) || (LRWPAN_IS_MAC(fcflsb)))  
                {
                    phy_pib.flags.bits.needACK = 1;
                    phy_pib.rcvSeqforACK = seq;
                    phy_pib.flags.bits.ackIsSending = 1; 
                }
            }
            #endif
            
            mac_sync_time.RxFractionSecond = macMcuOverflowCapture();//С������
            mac_sync_time.RxSecond= TAI_Second;//��������
            phyRxCallback();
            
            /* ����ռ� */
            // ֻӦ������ʱ��̬�����ڴ�
            // ǰ������в�����Ӧ��ʹ��SI4463_ReadBuf�����ݣ�����Ӧ��ʹ�ö�̬�ڴ�
            rx_frame = MemAlloc(rxBytes + 3);
            ptr      = rx_frame;
            if(rx_frame==NULL)              { return 0; }
            
            *ptr++   = rxBytes;
            for(i=0;i<rxBytes;i++) {
                *ptr++ = SI4463_ReadBuf[i+1];
            }

            /* �������� */
            macRxCallback(rx_frame, bRssi);
        }

do_rxflush:
        #undef  fcflsb
        #undef  fcfmsb
        #undef  dstmode
        #undef  srcmode

        Si4463_GetIntStatus(0,0,0,SI446x_Status); 
        SI4463_StartRx(RX_DATA_LENGTH);
    }
    else
    {
        Si4463_GetIntStatus(0x00,0,0,SI446x_Status); 
        SI4463_StartRx(RX_DATA_LENGTH);    
    }
    
    return 0;
}