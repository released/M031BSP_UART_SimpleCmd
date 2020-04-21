# M031BSP_UART_SimpleCmd
 M031BSP_UART_SimpleCmd

update @ 2020/04/21

base on project : M031BSP_UART_RX_UnknownLength , add UART command parsing , EVM : NK-M032SE

- CMD format : (if data length define as 4)

		HEADER 	func  	len		D0  	D1  	D2  	D3		CHK 	TAIL

- CHECKSUM = 0 - (byte 0 + ... byte 6) , if UARTCMD_DATA_LEN define 4 

- use eagleCom to verify by send UART command from PC , setting as below

![image](https://github.com/released/M031BSP_UART_SimpleCmd/blob/master/terminal_setting.jpg)

- example (press red circle to send each command)

- send TX command : \30\F5\04\30\32\39\35\07\36 , will parse correct function code and display

	- Function : 0xF5 , res = 295

- send TX command : \30\E4\04\0A\0B\0C\0D\BA\36 , will parse correct function code and display

	- Function : 0xE4 , 0x A , 0x B , 0x C , 0x D ,

- send TX command : \12\34\550A\0B\0C\0D\BD , will parse function code as incorrect and display 

	- Incorrect cmd format !!! 

	- 0x0000  12 34 55 30 41 0b 0c 0d bd 00 00 00 00 00 00 00   .4U0A...........


![image](https://github.com/released/M031BSP_UART_SimpleCmd/blob/master/terminal_example.jpg)


