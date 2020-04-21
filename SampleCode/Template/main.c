/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    A project template for M031 MCU.
 *
 * Copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "NuMicro.h"

#define FIFO_THRESHOLD 		(4)
#define RX_BUFFER_SIZE 		(128)
#define RX_TIMEOUT_CNT 		(20)

enum
{
    eUART_RX_Received_Data_Finish = 0,
    eUART_RX_Received_Data_NOT_Finish
};

volatile uint8_t g_au8UART_RX_Buffer[RX_BUFFER_SIZE] = {0}; // UART Rx received data Buffer (RAM)
volatile uint8_t g_bUART_RX_Received_Data_State = eUART_RX_Received_Data_NOT_Finish;
volatile uint8_t g_u8UART_RDA_Trigger_Cnt = 0; // UART RDA interrupt trigger times counter
volatile uint8_t g_u8UART_RXTO_Trigger_Cnt = 0; // UART RXTO interrupt trigger times counter

uint16_t g_u16UART_RX_length = 0;

typedef enum{
	flag_1ms = 0 ,
	
	flag_DEFAULT	
}Flag_Index;

uint8_t BitFlag = 0;
#define BitFlag_ON(flag)							(BitFlag|=flag)
#define BitFlag_OFF(flag)							(BitFlag&=~flag)
#define BitFlag_READ(flag)							((BitFlag&flag)?1:0)
#define ReadBit(bit)								(uint8_t)(1<<bit)

#define is_flag_set(idx)							(BitFlag_READ(ReadBit(idx)))
#define set_flag(idx,en)							( (en == 1) ? (BitFlag_ON(ReadBit(idx))) : (BitFlag_OFF(ReadBit(idx))))

/*
	CMD structure :
	header + function + data length + data0 + data1 + data2 + ... + checksum + tail

*/

#define CMD_HEADER								(0x30)
#define CMD_TAIL									(0x36)
#define CMD_DATA_LEN							(4)

typedef enum{
	Function_AAA = 0xF5 ,
	Function_BBB = 0xE4 ,
	Function_CCC = 0xD3 ,
	Function_DDD = 0xC2 ,
	Function_EEE = 0xB1 ,
	Function_FFF = 0xA0 ,	
	
}Function_Index;

typedef struct {
	uint8_t header;
	Function_Index function;
	uint8_t datalength;
	uint8_t data[CMD_DATA_LEN];
	uint8_t checksum;
	uint8_t tail;
	
}Cmd_Struct;

typedef enum{
	cmd_idx_header = 0 ,
	cmd_idx_function , 
	cmd_idx_datalength , 
	cmd_idx_data ,
	cmd_idx_checksum = cmd_idx_data + CMD_DATA_LEN,
	cmd_idx_tail ,

}Cmd_Index;

#define UARTCMD_CHKSUM_OFFSET					(sizeof(Cmd_Struct)-1)
#define UARTCMD_TOTAL_LENGTH					(sizeof(Cmd_Struct))

Cmd_Struct* cmd;


void  dump_buff_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02x ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}


/*
	CMD format
		0   	1   	2    	3   	4   	5   	6   	7		8
	HEADER 		func  	len		D0  	D1  	D2  	D3		CHK 	TAIL

	CHECKSUM = 0 - (byte 0 + ... byte 6) , if UARTCMD_DATA_LEN define 4 
*/
uint8_t GetChecksum(Cmd_Struct *cmd)
{
    uint16_t i = 0;
    uint8_t u8Checksum = 0;
    uint8_t* u8Ptr = (uint8_t*)cmd;
	
    for (i = 0 ; i < UARTCMD_CHKSUM_OFFSET - 1; i++)
    {
        u8Checksum += *(u8Ptr+i);
    }
	
    return (uint8_t) (0 - u8Checksum);
}

void clear_RX_Buffer(void)
{
	uint16_t i = 0;

	//reset buffer
	for(i = 0; i < RX_BUFFER_SIZE; i++)
	{
		g_au8UART_RX_Buffer[i] = 0;
	}

	//reset cmd structure
	cmd->header 		= 0x00;
	cmd->function 		= (Function_Index)	0x00;
	cmd->datalength		= 0x00;
	for (i = 0 ; i < CMD_DATA_LEN ; i++)
	{
		cmd->data[i] 	= 0x00;
	}
	cmd->checksum 		= 0x00;
	cmd->tail 			= 0x00;

}

/*
	CMD format
		0   	1   	2    	3   	4   	5   	6   	7		8
	HEADER 		func  	len		D0  	D1  	D2  	D3		CHK 	TAIL

*/

void Function_Process(Function_Index idx)
{
	uint16_t res = 0;
	switch(idx)
	{
		case Function_AAA:
		/*
			Function_AAA command template example :	
			\30			\F5		\4		\30		\32		\39		\35		\0x00 - (byte 0 + .. byte 6)	\36

			==> \30\F5\04\30\32\39\35\*07*\36	, res = 295
			==> \30\F5\04\31\34\34\33\*0B*\36	, res = 1443
		*/
		
		res = (cmd->data[0] -'0')*1000 +
			(cmd->data[1] -'0')*100 + 
			(cmd->data[2] -'0')*10 + 
			(cmd->data[3] -'0')*1;

		printf("\r\nFunction : 0x%2X , res = %2d\r\n", idx , res);
		
		break;

		case Function_BBB:
		/*
			Function_BBB command template example :	
			\30			\E4		\4		\0A		\0B		\0C		\0D		\0x00 - (byte 0 + .. byte 6)	\36

			==> \30\E4\04\0A\0B\0C\0D\*BA*\36
			==> \30\E4\04\10\20\30\40\*48*\36
		*/
		printf("\r\nFunction : 0x%2X , 0x%2X , 0x%2X , 0x%2X , 0x%2X , \r\n", idx , 
				cmd->data[0],
				cmd->data[1],
				cmd->data[2],
				cmd->data[3]);
		
		break;

		case Function_CCC:
		printf("\r\nFunction : 0x%2X , 0x%2X , 0x%2X , 0x%2X , 0x%2X , \r\n", idx , 
				cmd->data[0],
				cmd->data[1],
				cmd->data[2],
				cmd->data[3]);
		break;

		case Function_DDD:
		printf("\r\nFunction : 0x%2X , 0x%2X , 0x%2X , 0x%2X , 0x%2X , \r\n", idx , 
				cmd->data[0],
				cmd->data[1],
				cmd->data[2],
				cmd->data[3]);
		break;

		case Function_EEE:
		printf("\r\nFunction : 0x%2X , 0x%2X , 0x%2X , 0x%2X , 0x%2X , \r\n", idx , 
				cmd->data[0],
				cmd->data[1],
				cmd->data[2],
				cmd->data[3]);
		break;

		case Function_FFF:
		printf("\r\nFunction : 0x%2X , 0x%2X , 0x%2X , 0x%2X , 0x%2X , \r\n", idx , 
				cmd->data[0],
				cmd->data[1],
				cmd->data[2],
				cmd->data[3]);
		break;
		
	}
}

void ParseCmd_Process(void)
{
	uint16_t len = g_u16UART_RX_length;
    uint8_t checksumCal = 0;
	
	//copy buffer into cmd structure
	cmd = (Cmd_Struct*)	g_au8UART_RX_Buffer;

	//check packet header and tail if correct format
	if ((g_au8UART_RX_Buffer[cmd_idx_header] == CMD_HEADER) && 
		(g_au8UART_RX_Buffer[cmd_idx_tail] == CMD_TAIL))
	{
		checksumCal = GetChecksum(cmd);
	}
		
	//compare checksum correct or bypass this packet
	if (checksumCal == cmd->checksum)	
	{
		Function_Process(cmd->function);
	}
	else
	{
		/*
			if parse with incorrect format ,
			ex : 

			==> \12\34\550A\0B\0C\0D\BD
			==> \34\0B\01\00\00\00\00\F4\81
			==> \34\28\04\11\22\33\44\2A\81
		*/
		printf("\r\nIncorrect cmd format or checksum !!! \r\n");
		dump_buff_hex((uint8_t *)g_au8UART_RX_Buffer,len);
	}

		
}

void GPIO_Init (void)
{
    GPIO_SetMode(PB, BIT14, GPIO_MODE_OUTPUT);
}

void TMR3_IRQHandler(void)
{
//	static uint32_t LOG = 0;
	static uint16_t CNT = 0;
	
    if(TIMER_GetIntFlag(TIMER3) == 1)
    {
        TIMER_ClearIntFlag(TIMER3);

		if (CNT++ >= 1000)
		{		
			CNT = 0;
//        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			PB14 ^= 1;
		}		
    }
}

void TIMER3_Init(void)
{
    TIMER_Open(TIMER3, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER3);
    NVIC_EnableIRQ(TMR3_IRQn);	
    TIMER_Start(TIMER3);
}

void UART_Process(void)
{
	/* Wait to receive UART data */
	while(UART_RX_IDLE(UART0));

	/* Start to received UART data */
	g_bUART_RX_Received_Data_State = eUART_RX_Received_Data_NOT_Finish;        
	/* Wait for receiving UART message finished */
	while(g_bUART_RX_Received_Data_State != eUART_RX_Received_Data_Finish); 

//	printf("\r\nUART0 Rx Received Data : %s\r\n",g_au8UART_RX_Buffer);
//	printf("UART0 Rx Received Len : %d\r\n",g_u16UART_RX_length);	
//	printf("UART0 Rx RDA (Fifofull) interrupt times : %d\r\n",g_u8UART_RDA_Trigger_Cnt);
//	printf("UART0 Rx RXTO (Timeout) interrupt times : %d\r\n",g_u8UART_RXTO_Trigger_Cnt);

	ParseCmd_Process();
	clear_RX_Buffer();

	/* Reset UART interrupt parameter */
	UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
	g_u8UART_RDA_Trigger_Cnt = 0; // UART RDA interrupt times
	g_u8UART_RXTO_Trigger_Cnt = 0; // UART RXTO interrupt times
}

void UART02_IRQHandler(void)
{
    uint8_t i;
    static uint16_t u16UART_RX_Buffer_Index = 0;

	if ((UART_GET_INT_FLAG(UART0,UART_INTSTS_RDAINT_Msk)))
	{
        /* UART receive data available flag */
        
        /* Record RDA interrupt trigger times */
        g_u8UART_RDA_Trigger_Cnt++;
        
        /* Move the data from Rx FIFO to sw buffer (RAM). */
        /* Every time leave 1 byte data in FIFO for Rx timeout */
        for(i = 0 ; i < (FIFO_THRESHOLD - 1) ; i++)
        {
            g_au8UART_RX_Buffer[u16UART_RX_Buffer_Index] = UART_READ(UART0);
            u16UART_RX_Buffer_Index ++;

            if (u16UART_RX_Buffer_Index >= RX_BUFFER_SIZE) 
                u16UART_RX_Buffer_Index = 0;
        }	
	}
    else if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RXTOINT_Msk)) 
    {
        /* When Rx timeout flag is set to 1, it means there is no data needs to be transmitted. */

        /* Record Timeout times */
        g_u8UART_RXTO_Trigger_Cnt++;

        /* Move the last data from Rx FIFO to sw buffer. */
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
            g_au8UART_RX_Buffer[u16UART_RX_Buffer_Index] = UART_READ(UART0);
            u16UART_RX_Buffer_Index ++;
        }

        /* Clear UART RX parameter */
        UART_DISABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
		g_u16UART_RX_length = u16UART_RX_Buffer_Index;
        u16UART_RX_Buffer_Index = 0;
        g_bUART_RX_Received_Data_State = eUART_RX_Received_Data_Finish;
    }
	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, RX_TIMEOUT_CNT);

	/* Set UART FIFO RX interrupt trigger level to 4-bytes*/
    UART0->FIFO = ((UART0->FIFO & (~UART_FIFO_RFITL_Msk)) | UART_FIFO_RFITL_4BYTES);

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART02_IRQn);	

	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());


	UART_WAIT_TX_EMPTY(UART0);
}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable HIRC clock (Internal RC 48MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
//    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
	
    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
//    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
	
    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable UART0 clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_PCLK0, CLK_CLKDIV0_UART0(1));
	
    CLK_EnableModuleClock(TMR3_MODULE);
    CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3SEL_PCLK1, 0);
	

    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk))    |       \
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M031 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

    UART0_Init();

	GPIO_Init();

	TIMER3_Init();
	
    /* Got no where to go, just loop forever */
    while(1)
    {
		UART_Process();
    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
