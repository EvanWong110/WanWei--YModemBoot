/*******************************************************************************
** �ļ���: 		upload.c
** ����:		�ϴ��ļ�����غ���
** ����ļ�:	common.h
*******************************************************************************/

/* ����ͷ�ļ� *****************************************************************/
#include "common.h"

/*******************************************************************************
  * @��������	SerialUpload
  * @����˵��   ͨ�������ϴ�һ���ļ�
  * @�������   ��
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void SerialUpload(void)
{
    uint32_t status = 0;
	uint8_t  key = 0;
	
    SerialPutString("\n\n\rSelect Receive File ... (press any key to abort)\n\r");

	key = GetKey();
    if (key == CRC16)
    {
        //ͨ��ymodemЭ���ϴ�����
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
    else if(key == 0)//��ʱ��ת
	{
		Jump_To_App();
	}
    {
        SerialPutString("\r\n\nAborted by user.\n\r");
    }

}

/*******************************�ļ�����***************************************/
