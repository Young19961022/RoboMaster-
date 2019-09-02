#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"
#include "delay.h"

//����OLED_IIC��ʼ�ź�
void OLED_IIC_Start(void)
{
    OLED_SDA_OUT();     //sda�����
    OLED_IIC_SDA = 1;
    OLED_IIC_SCL = 1;
//	delay_us(1);
    OLED_IIC_SDA = 0; //START:when CLK is high,DATA change form high to low
//	delay_us(1);
    OLED_IIC_SCL = 0; //ǯסI2C���ߣ�׼�����ͻ��������
}
//����OLED_IICֹͣ�ź�
void OLED_IIC_Stop(void)
{
    OLED_SDA_OUT();//sda�����
    OLED_IIC_SCL = 0;
    OLED_IIC_SDA = 0;
    OLED_IIC_SCL = 1; //STOP:when CLK is high DATA change form low to high
//	delay_us(1);
    OLED_IIC_SDA = 1; //����I2C���߽����ź�
}
//�ȴ�Ӧ���źŵ���
//����ֵ��1������Ӧ��ʧ��
//        0������Ӧ��ɹ�
u8 OLED_IIC_Wait_Ack(void)
{
    u8 ucErrTime = 0;
    OLED_SDA_IN();      //SDA����Ϊ����
    OLED_IIC_SDA = 1;
    OLED_IIC_SCL = 1;
    while(OLED_READ_SDA)
    {
        ucErrTime++;
        if(ucErrTime > 250)
        {
            OLED_IIC_Stop();
            return 1;
        }
    }
    OLED_IIC_SCL = 0; //ʱ�����0
    return 0;
}

//OLED_IIC����һ���ֽ�
void OLED_IIC_Send_Byte(u8 txd)
{
    u8 t;
    OLED_SDA_OUT();
    OLED_IIC_SCL = 0; //����ʱ�ӿ�ʼ���ݴ���
    for(t = 0; t < 8; t++)
    {
        OLED_IIC_SDA = (txd & 0x80) >> 7;
        txd <<= 1;
//		delay_us(1);
        OLED_IIC_SCL = 1;
//		delay_us(1);
        OLED_IIC_SCL = 0;
//		delay_us(1);
    }
}

void I2C_WriteByte(u8 addr, u8 data)
{
    OLED_IIC_Start();
    OLED_IIC_Send_Byte(0x78);            //Slave address,SA0=0
    OLED_IIC_Wait_Ack();
    OLED_IIC_Send_Byte(addr);			//write command
    OLED_IIC_Wait_Ack();
    OLED_IIC_Send_Byte(data);
    OLED_IIC_Wait_Ack();
    OLED_IIC_Stop();
}
/**
 * @brief  WriteCmd����OLEDд������
 * @param  I2C_Command���������
 * @retval ��
 */
void WriteCmd(unsigned char I2C_Command)//д����
{
    I2C_WriteByte(0x00, I2C_Command);
}

/**
 * @brief  WriteDat����OLEDд������
 * @param  I2C_Data������
 * @retval ��
 */
void WriteDat(unsigned char I2C_Data)//д����
{
    I2C_WriteByte(0x40, I2C_Data);
}

//��ʼ��SSD1306
void OLED_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA, ENABLE );	//ʹ��GPIOBʱ��

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //�������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��

    OLED_IIC_SCL = 1;
    OLED_IIC_SDA = 1;


    WriteCmd(0xAE); //�ر���ʾ
    WriteCmd(0xD5); //����ʱ�ӷ�Ƶ����,��Ƶ��
    WriteCmd(0x50);   //[3:0],��Ƶ����;[7:4],��Ƶ��
    WriteCmd(0xA8); //��������·��
    WriteCmd(0X3F); //Ĭ��0X3F(1/64)
    WriteCmd(0xD3); //������ʾƫ��
    WriteCmd(0X00); //Ĭ��Ϊ0

    WriteCmd(0x40); //������ʾ��ʼ�� [5:0],����.

    WriteCmd(0x8D); //��ɱ�����
    WriteCmd(0x14); //bit2������/�ر�
    WriteCmd(0x20); //�����ڴ��ַģʽ
    WriteCmd(0x02); //[1:0],00���е�ַģʽ;01���е�ַģʽ;10,ҳ��ַģʽ;Ĭ��10;
    WriteCmd(0xA1); //���ض�������,bit0:0,0->0;1,0->127;
    WriteCmd(0xC0); //����COMɨ�跽��;bit3:0,��ͨģʽ;1,�ض���ģʽ COM[N-1]->COM0;N:����·��
    WriteCmd(0xDA); //����COMӲ����������
    WriteCmd(0x12); //[5:4]����

    WriteCmd(0x81); //�Աȶ�����
    WriteCmd(0xEF); //1~255;Ĭ��0X7F (��������,Խ��Խ��)
    WriteCmd(0xD9); //����Ԥ�������
    WriteCmd(0xf1); //[3:0],PHASE 1;[7:4],PHASE 2;
    WriteCmd(0xDB); //����VCOMH ��ѹ����
    WriteCmd(0x30); //[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;

    WriteCmd(0xA4); //ȫ����ʾ����;bit0:1,����;0,�ر�;(����/����)
    WriteCmd(0xA6); //������ʾ��ʽ;bit0:1,������ʾ;0,������ʾ
    WriteCmd(0xAF); //������ʾ

    OLED_Fill(0x00);
}
/**
 * @brief  OLED_SetPos�����ù��
 * @param  x,���xλ��
 *		   y�����yλ��
 * @retval ��
 */
void OLED_SetPos(unsigned char x, unsigned char y) //������ʼ������
{
    WriteCmd(0xb0 + y);
    WriteCmd(((x & 0xf0) >> 4) | 0x10);
    WriteCmd((x & 0x0f) | 0x01);
}

/**
 * @brief  OLED_Fill�����������Ļ
 * @param  fill_Data:Ҫ��������
* @retval ��
 */
void OLED_Fill(unsigned char fill_Data)//ȫ�����
{
    unsigned char m, n;
    for(m = 0; m < 8; m++)
    {
        WriteCmd(0xb0 + m);		//page0-page1
        WriteCmd(0x00);		//low column start address
        WriteCmd(0x10);		//high column start address
        for(n = 0; n < 128; n++)
        {
            WriteDat(fill_Data);
        }
    }
}

/**
 * @brief  OLED_CLS������
 * @param  ��
* @retval ��
 */
void OLED_CLS(void)//����
{
    OLED_Fill(0x00);
}


/**
 * @brief  OLED_ON����OLED�������л���
 * @param  ��
* @retval ��
 */
void OLED_ON(void)
{
    WriteCmd(0X8D);  //���õ�ɱ�
    WriteCmd(0X14);  //������ɱ�
    WriteCmd(0XAF);  //OLED����
}


/**
 * @brief  OLED_OFF����OLED���� -- ����ģʽ��,OLED���Ĳ���10uA
 * @param  ��
* @retval ��
 */
void OLED_OFF(void)
{
    WriteCmd(0X8D);  //���õ�ɱ�
    WriteCmd(0X10);  //�رյ�ɱ�
    WriteCmd(0XAE);  //OLED����
}

unsigned char reverse8( unsigned char c )
{
    c = ( c & 0x55 ) << 1 | ( c & 0xAA ) >> 1;
    c = ( c & 0x33 ) << 2 | ( c & 0xCC ) >> 2;
    c = ( c & 0x0F ) << 4 | ( c & 0xF0 ) >> 4;
    return c;
}
/**
 * @brief  OLED_ShowStr����ʾcodetab.h�е�ASCII�ַ�,��6*8��8*16��ѡ��
 * @param  x,y : ��ʼ������(x:0~127, y:0~7);
*					ch[] :- Ҫ��ʾ���ַ���;
*					TextSize : �ַ���С(1:6*8 ; 2:8*16)
* @retval ��
 */
void OLED_ShowStr(unsigned char x, unsigned char y, char ch[], unsigned char TextSize)
{
    unsigned char c = 0, i = 0, j = 0;
    switch(TextSize)
    {
    case 6:
    {
        while(ch[j] != '\0')
        {
            c = ch[j] - 32;
            if(x > 126)
            {
                x = 0;
                y++;
            }
            OLED_SetPos(x, 7-y);
            for(i = 0; i < 6; i++)
                WriteDat(reverse8(F6x8[c][i]));
            x += 6;
            j++;
        }
    }
    break;
    case 8:
    {
        while(ch[j] != '\0')
        {
            c = ch[j] - 32;
            if(x > 120)
            {
                x = 0;
                y++;
            }
            OLED_SetPos(x, 7-y);
            for(i = 0; i < 8; i++)
                WriteDat(reverse8(F8X16[c * 16 + i]));
            OLED_SetPos(x, 6-y);
            for(i = 0; i < 8; i++)
                WriteDat(reverse8(F8X16[c * 16 + i + 8]));
            x += 8;
            j++;
        }
    }
    break;
    }
}

/**
 * @brief  OLED_DrawBMP����ʾBMPλͼ
 * @param  x0,y0 :��ʼ������(x0:0~127, y0:0~7);
*					x1,y1 : ���Խ���(������)������(x1:1~128,y1:1~8)
* @retval ��
 */
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[])
{
    unsigned int j = 0;
    unsigned char x, y;

    if(y1 % 8 == 0)
        y = y1 / 8;
    else
        y = y1 / 8 + 1;
    for(y = y0; y < y1; y++)
    {
        OLED_SetPos(x0, y);
        for(x = x0; x < x1; x++)
        {
            WriteDat(BMP[j++]);
        }
    }
}








