/*******************************************************************************
** �ļ���: 		ymodem.c
** ����:		��Ymodem.c����ص�Э���ļ�
                ����ӳ����ն˽�������(ʹ��YmodemЭ��)���������ݼ��ص��ڲ�RAM�С�
                ����������������������ݱ�̵�Flash�У����������������ʾ����
** ����ļ�:	stm32f10x.h
*******************************************************************************/

/* ����ͷ�ļ� *****************************************************************/

#include "common.h"
#include "stm32f10x_flash.h"

/* �������� -----------------------------------------------------------------*/
uint8_t file_name[FILE_NAME_LENGTH];
//�û�����Flashƫ��
uint32_t FlashDestination = ApplicationAddress;
uint16_t PageSize = PAGE_SIZE;
uint32_t EraseCounter = 0x0;
uint32_t NbrOfPage = 0;
FLASH_Status FLASHStatus = FLASH_COMPLETE;
uint32_t RamSource;
extern uint8_t tab_1024[1024];

/*******************************************************************************
  * @��������	Receive_Byte
  * @����˵��   �ӷ��Ͷ˽���һ���ֽ�
  * @�������   c: �����ַ�
                timeout: ��ʱʱ��
  * @�������   ��
  * @���ز���   ���յĽ��
                0���ɹ�����
                1��ʱ�䳬ʱ
*******************************************************************************/
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
    while (timeout-- > 0)
    {
        if (SerialKeyPressed(c) == 1)//����1��ʾ�յ�
        {
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
  * @��������	Send_Byte
  * @����˵��   ����һ���ַ�
  * @�������   c: ���͵��ַ�
  * @�������   ��
  * @���ز���   ���͵Ľ��
                0���ɹ�����
*******************************************************************************/
static uint32_t Send_Byte (uint8_t c)
{
    SerialPutChar(c);
    return 0;
}

/*******************************************************************************
  * @��������	Receive_Packet
  * @����˵��   �ӷ��Ͷ˻��ڴ��ڷ�ʽ����һ�����ݰ�
  * @�������   data �����յ������ݻ�����
                length�����յ����ݳ���
                timeout ����ʱʱ��
  * @�������   length
                0 ��źͲ���У�鲻�ɹ�,EOT
                -1���ͷ���ֹ����
                >0 ���������ݰ�����
  * @���ز���   ���յĽ��
                0: ��������
                -1: ��ʱ�������ݰ�����
                1: �û�ȡ��
*******************************************************************************/
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
    uint16_t i, packet_size;
    uint8_t c;
    *length = 0;
    if (Receive_Byte(&c, timeout) != 0)
    {
        return -1;//��ʱ����-1
    }
    switch (c)//���յ��ĵ�һ���ֽ�
    {
		case SOH://���ݰ���ʼ�����
			packet_size = PACKET_SIZE;
			break;
		case STX://���Ŀ�ʼ
			packet_size = PACKET_1K_SIZE;
			break;
		case EOT://���Ĵ������
			return 0;
		case CA://���ͷ���ֹ����
			if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
			{
				*length = -1;
				return 0;
			}
			else
			{
				return -1;
			}
		case ABORT1:
		case ABORT2:return 1;
		default:return -1;
    }
	
    *data = c;//����һ���ֽڴ��뻺����
    for (i = 1; i < (packet_size + PACKET_OVERHEAD); i ++)//����һ֡��ʣ�µ��ֽ�
    {
        if (Receive_Byte(data + i, timeout) != 0)
        {
            return -1;
        }
    }
    if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))//У����źͲ���
    {
        return -1;
    }
	
    *length = packet_size;//��ȡ���ݰ�����,��źͲ���У�鲻�ɹ�ʱΪ0
    return 0;
}

/*******************************************************************************
  * @��������	Ymodem_Receive
  * @����˵��   ͨ�� ymodemЭ�����һ���ļ�
  * @�������   buf: �׵�ַָ��
  * @�������   ��
  * @���ز���   �ļ�����
*******************************************************************************/
int32_t Ymodem_Receive (uint8_t *buf)
{
	uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];//���һ֡���������ݰ�
	uint8_t file_size[FILE_SIZE_LENGTH], *file_ptr;//��ŵ�һ֡���ݰ��е��ļ����ƺ��ļ���С
	uint8_t *buf_ptr;//���һ֡���ݰ��е����ݲ���,ָ�����buf
    int32_t i, j, errors, session_begin;
	int32_t packet_length;//���յ�������֡����128 �� 1024
	int32_t session_done;//������ɱ�־
	int32_t file_done;//һ֡�ļ�������ɱ�־
	int32_t packets_received;//���յ������ݰ�����,�����ݰ��е���ŶԱ�
	int32_t size = 0;//һ֡�ļ����ֽ�����
	static  uint8_t eotcount = 0;
		
    //��ʼ��Flash��ַ����
    FlashDestination = ApplicationAddress;

    for (session_done = 0, errors = 0, session_begin = 0; ;)
    {
        for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
        {
            switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))//0 1 -1
            {
              case 0: //��������,��ȷ���յ�һ֡���ݰ�
			  {
				errors = 0;
                switch (packet_length)//0 -1 >0
                {        
					case - 1://���ͷ���ֹ����
					{
						Send_Byte(ACK);
						return 0;       
					}break;        
					case 0://EOT
					{
						if(eotcount == 0)//��һ��
						{
							Delay_ms(1);//���������ʱһ��,�����Ͷ�׼������,��Ȼ�����
							eotcount++;
							Send_Byte(NAK);
						}				
						else //�ڶ���	
						{
							Delay_ms(1);//���������ʱһ��,�����Ͷ�׼������,��Ȼ�����
							Send_Byte(ACK);	
							Send_Byte(CRC16);
						}		
					}break;                   
					default://���������ݰ�
					{
						//������ݰ��е��������յ������ݰ�����,  ����Ϊ�������һ������
						if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
						{
							Delay_ms(1);//���������ʱһ��,�����Ͷ�׼������,��Ȼ�����
							Send_Byte(ACK);	
							session_done = 1;
							file_done = 1;
						}
						else//��ȷ�����ݰ�
						{
							if (packets_received == 0)//��һ������
							{			
								if (packet_data[PACKET_HEADER] != 0)//��ȥ3�ֽڵ��ײ�
								{
									//�ļ������ݰ���Ч��������
									for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
									{
										file_name[i++] = *file_ptr++;
									}
									file_name[i++] = '\0';
									for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
									{
										file_size[i++] = *file_ptr++;
									}
									file_size[i++] = '\0';
									Str2Int(file_size, &size);

									//�������ݰ��Ƿ����,����flash�洢��С
									if (size > (FLASH_SIZE - 1))
									{
										//����
										Send_Byte(CA);
										Send_Byte(CA);
										return -1;
									}

									//������Ҫ����Flash��ҳ
									NbrOfPage = FLASH_PagesMask(size);
									//����Flash
									for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
									{
										FLASHStatus = FLASH_ErasePage(FlashDestination + (PageSize * EraseCounter));
									}
									Send_Byte(ACK);//��һ֡���ݰ��������,���շ������ͷ�����ACK C
									Send_Byte(CRC16);
								}
								else//�ļ������ݰ��գ���������
								{
									//Send_Byte(ACK);
									Send_Byte(CA);
									Send_Byte(CA);
									file_done = 1;
									session_done = 1;
									break;
								}
							}
							else//���ݰ�
							{
								memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
								RamSource = (uint32_t)buf;
								for (j = 0; (j < packet_length) && (FlashDestination <  ApplicationAddress + size); j += 4)
								{
									//�ѽ��յ������ݱ�д��Flash��
									FLASH_ProgramWord(FlashDestination, *(uint32_t*)RamSource);
									if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
									{
										//����
										Send_Byte(CA);
										Send_Byte(CA);
										return -2;
									}
									FlashDestination += 4;
									RamSource += 4;
								}
								Send_Byte(ACK);
							}
							
							packets_received ++;//ÿ�յ�һ����ȷ�����ݼ�1
							session_begin = 1;
						}
					}break;	
                }//end  switch (packet_length) 
			  }break;
				
              case 1://�û�ȡ��
			  {
				Send_Byte(CA);//�����ַ�'CAN'
                Send_Byte(CA);
                return -3;
			  }break;
			
              default://��ʱ ���� ���ݰ����� ���� ��źͲ���У�鲻�ɹ�
			  {
				if (session_begin > 0)
				{
					errors ++;
				}
				if (errors > MAX_ERRORS)
				{
					Send_Byte(CA);
					Send_Byte(CA);
					return 0;
				}
				Send_Byte(CRC16);//����'C',����Ϊ�տ�ʼ��������ݰ�����ʱ���շ������ͷ����͵������ַ�,����Ҫ
			  } break;	
            }//end switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
			
            if (file_done != 0)//�ļ�������ֹ
            {
                break;
            }			
        }//for 2 ��ѭ��
		
        if (session_done != 0)//������ֹ
        {
            break;
        }
    }//for 1 ��ѭ��
	
    return (int32_t)size;
}

/*******************************************************************************
  * @��������	Ymodem_CheckResponse
  * @����˵��   ͨ�� ymodemЭ������Ӧ
  * @�������   c
  * @�������   ��
  * @���ز���   0
*******************************************************************************/
int32_t Ymodem_CheckResponse(uint8_t c)
{
    return 0;
}

/*******************************************************************************
  * @��������	Ymodem_PrepareIntialPacket
  * @����˵��   ׼����һ�����ݰ�
  * @�������   data�����ݰ�
                fileName ���ļ���
                length ������
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
    uint16_t i, j;
    uint8_t file_ptr[10];

    //����ͷ3�����ݰ�
    data[0] = SOH;
    data[1] = 0x00;
    data[2] = 0xff;
    //�ļ������ݰ���Ч����
    for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH); i++)
    {
        data[i + PACKET_HEADER] = fileName[i];
    }

    data[i + PACKET_HEADER] = 0x00;

    Int2Str (file_ptr, *length);
    for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
    {
        data[i++] = file_ptr[j++];
    }

    for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
    {
        data[j] = 0;
    }
}

/*******************************************************************************
  * @��������	Ymodem_PreparePacket
  * @����˵��   ׼�����ݰ�
  * @�������   SourceBuf������Դ����
                data�����ݰ�
                pktNo �����ݰ����
                sizeBlk ������
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
    uint16_t i, size, packetSize;
    uint8_t* file_ptr;

    //����ͷ3�����ݰ�
    packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
    size = sizeBlk < packetSize ? sizeBlk :packetSize;
    if (packetSize == PACKET_1K_SIZE)
    {
        data[0] = STX;
    }
    else
    {
        data[0] = SOH;
    }
    data[1] = pktNo;
    data[2] = (~pktNo);
    file_ptr = SourceBuf;

    //�ļ������ݰ���Ч����
    for (i = PACKET_HEADER; i < size + PACKET_HEADER; i++)
    {
        data[i] = *file_ptr++;
    }
    if ( size  <= packetSize)
    {
        for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
        {
            data[i] = 0x1A; //����
        }
    }
}

/*******************************************************************************
  * @��������	UpdateCRC16
  * @����˵��   �����������ݵģãң�У��
  * @�������   crcIn
                byte
  * @�������   ��
  * @���ز���   �ãң�У��ֵ
*******************************************************************************/
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
    uint32_t crc = crcIn;
    uint32_t in = byte|0x100;
    do
    {
        crc <<= 1;
        in <<= 1;
        if (in&0x100)
            ++crc;
        if (crc&0x10000)
            crc ^= 0x1021;
    }
    while (!(in&0x10000));
    return crc&0xffffu;
}

/*******************************************************************************
  * @��������	UpdateCRC16
  * @����˵��   �����������ݵģãң�У��
  * @�������   data ������
                size ������
  * @�������   ��
  * @���ز���   �ãң�У��ֵ
*******************************************************************************/
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
    uint32_t crc = 0;
    const uint8_t* dataEnd = data+size;
    while (data<dataEnd)
        crc = UpdateCRC16(crc,*data++);

    crc = UpdateCRC16(crc,0);
    crc = UpdateCRC16(crc,0);
    return crc&0xffffu;
}


/*******************************************************************************
  * @��������	CalChecksum
  * @����˵��   ����YModem���ݰ����ܴ�С
  * @�������   data ������
                size ������
  * @�������   ��
  * @���ز���   ���ݰ����ܴ�С
*******************************************************************************/
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
    uint32_t sum = 0;
    const uint8_t* dataEnd = data+size;
    while (data < dataEnd )
        sum += *data++;
    return sum&0xffu;
}

/*******************************************************************************
  * @��������	Ymodem_SendPacket
  * @����˵��   ͨ��ymodemЭ�鴫��һ�����ݰ�
  * @�������   data �����ݵ�ַָ��
                length������
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
    uint16_t i;
    i = 0;
    while (i < length)
    {
        Send_Byte(data[i]);
        i++;
    }
}

/*******************************************************************************
  * @��������	Ymodem_Transmit
  * @����˵��   ͨ��ymodemЭ�鴫��һ���ļ�
  * @�������   buf �����ݵ�ַָ��
                sendFileName ���ļ���
                sizeFile���ļ�����
  * @�������   ��
  * @���ز���   �Ƿ�ɹ�
                0���ɹ�
*******************************************************************************/
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{

    uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
    uint8_t FileName[FILE_NAME_LENGTH];
    uint8_t *buf_ptr, tempCheckSum ;
    uint16_t tempCRC, blkNumber;
    uint8_t receivedC[2], CRC16_F = 0, i;
    uint32_t errors, ackReceived, size = 0, pktSize;

    errors = 0;
    ackReceived = 0;
    for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
    {
        FileName[i] = sendFileName[i];
    }
    CRC16_F = 1;

    //׼����һ�����ݰ�
    Ymodem_PrepareIntialPacket(&packet_data[0], FileName, &sizeFile);

    do
    {
        //�������ݰ�
        Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
        //����CRCУ��
        if (CRC16_F)
        {
            tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
            Send_Byte(tempCRC >> 8);
            Send_Byte(tempCRC & 0xFF);
        }
        else
        {
            tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
            Send_Byte(tempCheckSum);
        }

        //�ȴ���Ӧ
        if (Receive_Byte(&receivedC[0], 10000) == 0)
        {
            if (receivedC[0] == ACK)
            {
                //���ݰ���ȷ����
                ackReceived = 1;
            }
        }
        else
        {
            errors++;
        }
    } while (!ackReceived && (errors < 0x0A));

    if (errors >=  0x0A)
    {
        return errors;
    }
    buf_ptr = buf;
    size = sizeFile;
    blkNumber = 0x01;

    //1024�ֽڵ����ݰ�����
    while (size)
    {
        //׼����һ�����ݰ�
        Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
        ackReceived = 0;
        receivedC[0]= 0;
        errors = 0;
        do
        {
            //������һ�����ݰ�
            if (size >= PACKET_1K_SIZE)
            {
                pktSize = PACKET_1K_SIZE;

            }
            else
            {
                pktSize = PACKET_SIZE;
            }
            Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
            //����CRCУ��
            if (CRC16_F)
            {
                tempCRC = Cal_CRC16(&packet_data[3], pktSize);
                Send_Byte(tempCRC >> 8);
                Send_Byte(tempCRC & 0xFF);
            }
            else
            {
                tempCheckSum = CalChecksum (&packet_data[3], pktSize);
                Send_Byte(tempCheckSum);
            }

            //�ȴ���Ӧ
            if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
            {
                ackReceived = 1;
                if (size > pktSize)
                {
                    buf_ptr += pktSize;
                    size -= pktSize;
                    if (blkNumber == (FLASH_IMAGE_SIZE/1024))
                    {
                        return 0xFF; //����
                    }
                    else
                    {
                        blkNumber++;
                    }
                }
                else
                {
                    buf_ptr += pktSize;
                    size = 0;
                }
            }
            else
            {
                errors++;
            }
        } while (!ackReceived && (errors < 0x0A));
        //���û��Ӧ10�ξͷ��ش���
        if (errors >=  0x0A)
        {
            return errors;
        }

    }
    ackReceived = 0;
    receivedC[0] = 0x00;
    errors = 0;
    do
    {
        Send_Byte(EOT);
        //���� (EOT);
        //�ȴ���Ӧ
        if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
        {
            ackReceived = 1;
        }
        else
        {
            errors++;
        }
    } while (!ackReceived && (errors < 0x0A));

    if (errors >=  0x0A)
    {
        return errors;
    }
    //׼�����һ����
    ackReceived = 0;
    receivedC[0] = 0x00;
    errors = 0;

    packet_data[0] = SOH;
    packet_data[1] = 0;
    packet_data [2] = 0xFF;

    for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
    {
        packet_data [i] = 0x00;
    }

    do
    {
        //�������ݰ�
        Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
        //����CRCУ��
        tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
        Send_Byte(tempCRC >> 8);
        Send_Byte(tempCRC & 0xFF);

        //�ȴ���Ӧ
        if (Receive_Byte(&receivedC[0], 10000) == 0)
        {
            if (receivedC[0] == ACK)
            {
                //��������ȷ
                ackReceived = 1;
            }
        }
        else
        {
            errors++;
        }

    } while (!ackReceived && (errors < 0x0A));
    //���û��Ӧ10�ξͷ��ش���
    if (errors >=  0x0A)
    {
        return errors;
    }

    do
    {
        Send_Byte(EOT);
        //���� (EOT);
        //�ȴ���Ӧ
        if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
        {
            ackReceived = 1;
        }
        else
        {
            errors++;
        }
    } while (!ackReceived && (errors < 0x0A));

    if (errors >=  0x0A)
    {
        return errors;
    }
    return 0;//�ļ�����ɹ�
}

/*******************************�ļ�����***************************************/
