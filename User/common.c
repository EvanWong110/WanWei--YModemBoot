/*******************************************************************************
** �ļ���: 		common.c
** ����:		��ʾ���˵������˵�����ʾһϵ�в���������ض������ļ���
                ִ��Ӧ�ó����Լ���ֹд����(�������Flash��д����)��
** ����ļ�:	common.h��ymodem.h

*******************************************************************************/

/* ����ͷ�ļ� *****************************************************************/
#include "common.h"
#include "ymodem.h"

/* ���� ----------------------------------------------------------------------*/
pFunction Jump_To_Application;
uint32_t  JumpAddress;
uint32_t  BlockNbr = 0, UserMemoryMask = 0;
__IO uint32_t   FlashProtection = 0;
extern uint32_t FlashDestination;




/*******************************************************************************
  * @��������	Delay_us
  * @����˵��   ��ʱnus
  * @�������   nusΪҪ��ʱ��us��.
  * @���ز���   ��
*******************************************************************************/
void Delay_us(u32 nus)
{		
	u32 temp;	
	u32 fac_us     = SystemCoreClock/8000000;			//Ϊϵͳʱ�ӵ�1/8  	
	
	SysTick->LOAD  = nus*fac_us; 					//ʱ�����	  		 
	SysTick->VAL   = 0x00;        					//��ռ�����
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk ;	//��ʼ����	  
	do
	{
		temp=SysTick->CTRL;
	}
	while((temp&0x01)&&!(temp&(1<<16)));		//�ȴ�ʱ�䵽��   
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;	//�رռ�����
	SysTick->VAL = 0X00;      					 //��ռ�����	 
}

/*******************************************************************************
  * @��������	Delay_ms
  * @����˵��   ��ʱnms
  * @�������   nusΪҪ��ʱ��us��.
  * @���ز���   ��
  * SysTick->LOADΪ24λ�Ĵ���,����,�����ʱΪ:
  * nms<=0xffffff*8*1000/SYSCLK
  * SYSCLK��λΪHz,nms��λΪms
  * ��72M������,nms<=1864 
*******************************************************************************/
void Delay_ms(u16 nms)
{	 		  	  
	u32 temp;		
	u32	fac_ms = SystemCoreClock/8000;			//Ϊϵͳʱ�ӵ�1/8  	
	
	SysTick->LOAD  = (u32)nms*fac_ms;				//ʱ�����(SysTick->LOADΪ24bit)
	SysTick->VAL   = 0x00;							//��ռ�����
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk ;	//��ʼ����  
	do
	{
		temp=SysTick->CTRL;
	}
	while((temp&0x01) &&! (temp&(1<<16)));		//�ȴ�ʱ�䵽��   
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;	//�رռ�����
	SysTick->VAL   = 0X00;       					//��ռ�����	  	    
} 


/*******************************************************************************
  * @��������	Int2Str
  * @����˵��   ��������ת���ַ���
  * @�������   intnum:����
  * @�������   str��ת��Ϊ���ַ���
  * @���ز���   ��
*******************************************************************************/
void Int2Str(uint8_t* str, int32_t intnum)
{
    uint32_t i, Div = 1000000000, j = 0, Status = 0;

    for (i = 0; i < 10; i++)
    {
        str[j++] = (intnum / Div) + 48;

        intnum = intnum % Div;
        Div /= 10;
        if ((str[j-1] == '0') & (Status == 0))
        {
            j = 0;
        }
        else
        {
            Status++;
        }
    }
}

/*******************************************************************************
  * @��������	Str2Int
  * @����˵��   �ַ���ת������
  * @�������   inputstr:��ת�����ַ���
  * @�������   intnum��ת�õ�����
  * @���ز���   ת�����
                1����ȷ
                0������
*******************************************************************************/
uint32_t Str2Int(uint8_t *inputstr, int32_t *intnum)
{
    uint32_t i = 0, res = 0;
    uint32_t val = 0;

    if (inputstr[0] == '0' && (inputstr[1] == 'x' || inputstr[1] == 'X'))
    {
        if (inputstr[2] == '\0')
        {
            return 0;
        }
        for (i = 2; i < 11; i++)
        {
            if (inputstr[i] == '\0')
            {
                *intnum = val;
                //����1
                res = 1;
                break;
            }
            if (ISVALIDHEX(inputstr[i]))
            {
                val = (val << 4) + CONVERTHEX(inputstr[i]);
            }
            else
            {
                //��Ч���뷵��0
                res = 0;
                break;
            }
        }

        if (i >= 11)
        {
            res = 0;
        }
    }
    else//���10Ϊ2����
    {
        for (i = 0; i < 11; i++)
        {
            if (inputstr[i] == '\0')
            {
                *intnum = val;
                //����1
                res = 1;
                break;
            }
            else if ((inputstr[i] == 'k' || inputstr[i] == 'K') && (i > 0))
            {
                val = val << 10;
                *intnum = val;
                res = 1;
                break;
            }
            else if ((inputstr[i] == 'm' || inputstr[i] == 'M') && (i > 0))
            {
                val = val << 20;
                *intnum = val;
                res = 1;
                break;
            }
            else if (ISVALIDDEC(inputstr[i]))
            {
                val = val * 10 + CONVERTDEC(inputstr[i]);
            }
            else
            {
                //��Ч���뷵��0
                res = 0;
                break;
            }
        }
        //����10λ��Ч������0
        if (i >= 11)
        {
            res = 0;
        }
    }

    return res;
}



/*******************************************************************************
  * @��������	SerialKeyPressed
  * @����˵��   ���Գ����ն��Ƿ��а�������
  * @�������   key:����
  * @�������   ��
  * @���ز���   1����ȷ
                0������
*******************************************************************************/
uint32_t SerialKeyPressed(uint8_t *key)
{

    if ( USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
    {
		*key = USART_ReceiveData(USART1);
		//USART_ClearFlag(USART1,USART_FLAG_RXNE);
        return 1;
    }
    else
    {
        return 0;
    }
}

/*******************************************************************************
  * @��������	GetKey
  * @����˵��   ͨ�������жϻ�ȥ����
  * @�������   ��
  * @�������   ��
  * @���ز���   ���µļ���
*******************************************************************************/
uint8_t GetKey(void)
{
    uint8_t key = 0,count = 10;
	char buf[50] = {0};
	
    //�ȴ���������
    while (count--)
    {
        if (SerialKeyPressed((uint8_t*)&key)) break;
		Delay_ms(1000);
		sprintf(buf,"remain time : %d S\r\n\n",count);
		SerialPutString(buf);
    }
	
    return key;

}

/*******************************************************************************
  * @��������	SerialPutChar
  * @����˵��   ���ڷ���һ���ַ�
  * @�������   C:�跢�͵��ַ�
  * @�������   i��
  * @���ز���   ��
*******************************************************************************/
void SerialPutChar(uint8_t c)
{
    USART_SendData(USART1, c);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
    {
		//USART_ClearFlag(USART1,USART_FLAG_TXE);
    }
}

/*******************************************************************************
  * @��������	SerialPutChar
  * @����˵��   ���ڷ���һ���ַ���
  * @�������   *s:�跢�͵��ַ���
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void Serial_PutString(uint8_t *s)
{
    while (*s != '\0')
    {
        SerialPutChar(*s);
        s++;
    }
}




/*******************************************************************************
  * @��������	FLASH_PagesMask
  * @����˵��   ����Falshҳ
  * @�������   Size:�ļ�����
  * @�������   ��
  * @���ز���   ҳ������
*******************************************************************************/
uint32_t FLASH_PagesMask(__IO uint32_t Size)
{
    uint32_t pagenumber = 0x0;
    uint32_t size = Size;

    if ((size % PAGE_SIZE) != 0)
    {
        pagenumber = (size / PAGE_SIZE) + 1;
    }
    else
    {
        pagenumber = size / PAGE_SIZE;
    }
    return pagenumber;

}

/*******************************************************************************
  * @��������	FLASH_DisableWriteProtectionPages
  * @����˵��   �Ӵ�Flashд����
  * @�������   ��
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void FLASH_DisableWriteProtectionPages(void)
{
    uint32_t useroptionbyte = 0, WRPR = 0;
    uint16_t var1 = OB_IWDG_SW, var2 = OB_STOP_NoRST, var3 = OB_STDBY_NoRST;
    FLASH_Status status = FLASH_BUSY;

    WRPR = FLASH_GetWriteProtectionOptionByte();

    //�����Ƿ�д����
    if ((WRPR & UserMemoryMask) != UserMemoryMask)
    {
        useroptionbyte = FLASH_GetUserOptionByte();

        UserMemoryMask |= WRPR;

        status = FLASH_EraseOptionBytes();

        if (UserMemoryMask != 0xFFFFFFFF)
        {
            status = FLASH_EnableWriteProtection((uint32_t)~UserMemoryMask);
        }
        //�ô�ѡ�����Ƿ��б��
        if ((useroptionbyte & 0x07) != 0x07)
        {
            //���±���ѡ����
            if ((useroptionbyte & 0x01) == 0x0)
            {
                var1 = OB_IWDG_HW;
            }
            if ((useroptionbyte & 0x02) == 0x0)
            {
                var2 = OB_STOP_RST;
            }
            if ((useroptionbyte & 0x04) == 0x0)
            {
                var3 = OB_STDBY_RST;
            }

            FLASH_UserOptionByteConfig(var1, var2, var3);
        }

        if (status == FLASH_COMPLETE)
        {
            SerialPutString("Write Protection disabled...\r\n");

            SerialPutString("...and a System Reset will be generated to re-load the new option bytes\r\n");
            //ϵͳ��λ���¼���ѡ����
            NVIC_SystemReset();
        }
        else
        {
            SerialPutString("Error: Flash write unprotection failed...\r\n");
        }
    }
    else
    {
        SerialPutString("Flash memory not write protected\r\n");
    }
}


/*******************************************************************************
  * @��������	Jump_To_App
  * @����˵��   ��ת��Ӧ�ó���
  * @�������   ��
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void Jump_To_App(void)
{
	if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
	{
		SerialPutString("Execute user Program\r\n\n");
		//��ת���û�����
		JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
		Jump_To_Application = (pFunction) JumpAddress;

		//��ʼ���û�����Ķ�ջָ��
		__set_MSP(*(__IO uint32_t*) ApplicationAddress);
		Jump_To_Application();
	}
	else
	{
		SerialPutString("no user Program\r\n\n");
	}
}

/*******************************************************************************
  * @��������	Main_Menu
  * @����˵��   ��ʾ�˵����ڳ����ն�
  * @�������   ��
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void Main_Menu(void)
{
    uint8_t key = 0;
    BlockNbr = (FlashDestination - 0x08000000) >> 12;

 
#if defined (STM32F10X_MD) || defined (STM32F10X_MD_VL)
    UserMemoryMask = ((uint32_t)~((1 << BlockNbr) - 1));
#else /* USE_STM3210E_EVAL */
    if (BlockNbr < 62)
    {
        UserMemoryMask = ((uint32_t)~((1 << BlockNbr) - 1));
    }
    else
    {
        UserMemoryMask = ((uint32_t)0x80000000);
    }
#endif /* (STM32F10X_MD) || (STM32F10X_MD_VL) */

    if ((FLASH_GetWriteProtectionOptionByte() & UserMemoryMask) != UserMemoryMask)
    {
        FlashProtection = 1;
    }
    else
    {
        FlashProtection = 0;
    }

	SerialPutString("\r\n======================== Main Menu =========================\r\n\n");
	SerialPutString("  Download New Program To   the STM32F10x Internal Flash ------- 1\r\n\n");
	SerialPutString("  Upload       Program From the STM32F10x Internal Flash ------- 2\r\n\n");
	SerialPutString("  Execute The old Program -------------------------------------- 3\r\n\n");
	if (FlashProtection != 0)
	{
		SerialPutString("  Disable the write protection ------------------------- 4\r\n\n");
	}
	SerialPutString("=================================================================\r\n\n");
		
	
    while (1)
    {

        key = GetKey();

        if (key == 0x31)//���س���
        {
            SerialDownload();
        }
        else if (key == 0x32)//�ϴ�����
        {
            SerialUpload();
        }
        else if (key == 0x33 || key == 0)//�ֶ���ת �� ��ʱ��ת
        {
			Jump_To_App();
        }
        else if ((key == 0x34) && (FlashProtection == 1))//���д����
        {
            FLASH_DisableWriteProtectionPages();
        }
		else//�ն��豸�������
        {
            if (FlashProtection == 0)
            {
                SerialPutString("Invalid Number ! ==> The number should be either 1, 2 or 3\r");
            }
            else
            {
                SerialPutString("Invalid Number ! ==> The number should be either 1, 2, 3 or 4\r");
            }
        }
    }
}


/*******************************�ļ�����***************************************/
