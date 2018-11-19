/*******************************************************************************
** 文件名: 		upload.c
** 功能:		上传文件的相关函数
** 相关文件:	common.h
*******************************************************************************/

/* 包含头文件 *****************************************************************/
#include "common.h"

/*******************************************************************************
  * @函数名称	SerialUpload
  * @函数说明   通过串口上传一个文件
  * @输入参数   无
  * @输出参数   无
  * @返回参数   无
*******************************************************************************/
void SerialUpload(void)
{
    uint32_t status = 0;
	uint8_t  key = 0;
	
    SerialPutString("\n\n\rSelect Receive File ... (press any key to abort)\n\r");

	key = GetKey();
    if (key == CRC16)
    {
        //通过ymodem协议上传程序
        status = Ymodem_Transmit((uint8_t*)ApplicationAddress, (const uint8_t*)"UploadedFlashImage.bin", FLASH_IMAGE_SIZE);

        if (status != 0)
        {
            SerialPutString("\n\rError Occured while Transmitting File\n\r");
        }
        else
        {
            SerialPutString("\n\rFile Trasmitted Successfully \n\r");
        }
    }
    else if(key == 0)//超时跳转
	{
		Jump_To_App();
	}
    {
        SerialPutString("\r\n\nAborted by user.\n\r");
    }

}

/*******************************文件结束***************************************/
