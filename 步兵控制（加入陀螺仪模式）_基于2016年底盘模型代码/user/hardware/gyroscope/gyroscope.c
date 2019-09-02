/**
  ****************************(C) COPYRIGHT 2019 IronSprit***********************
  * @file       gyroscope.c/h

  * @brief      ��������һ�鵥Ƭ��ȫ�����з��������������ݣ����ô���7�����ж���DMA˫
	              ���������䷽ʽ��ԼCPU��Դ��

  * @note       1����������ͨ�����ڿ����ж�����������freeRTOS����
	*             2����static�����ⲿ����,��ı������ӷ�ʽ��ʹ��ֻ�ڱ��ļ��ڲ���Ч��
	                 �������ļ��������ӻ����øñ�����
	* 					  3��ʹ��static���ں�������ʱ���Ժ��������ӷ�ʽ����Ӱ�죬ʹ�ú���
								   ֻ�ڱ��ļ��ڲ���Ч���������ļ��ǲ��ɼ��ġ����õ����������ļ���
									 ͬ�������������ţ�����Ҳ�ǶԺ��������һ�ֱ������ơ�
	*							4��ע�⿴����7�����ж������ʱ��ֵ�洢��0��1
	              5��structĬ�������Ա�Ķ��뷽ʽ��Ϊ����Ķ��뷽ʽ����union��struct
								   ����ʱ���ͻᵼ������������һֱ����ǿ��1�ֽڶ����������

  * @history
  *  Version       Date            Author          status
  *  V3.0.0       2019-6-30        Young            ���
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 IronSprit************************
*/

#include "gyroscope.h"
#include "string.h"

MPU6050_Pack_t MPU6050_Pack[2];
static float INS_Angle[3] = {0.0f};      //ŷ���� ��λ��,yaw,pitch,roll
static float INS_gyro[3]  = {0.0f};      //���ٶ�dps

/**
  * @brief          ����DMA���ж�����
  * @author         Young
  * @param[in]      ������0��ַ
  * @param[in]      ������1��ַ
  * @param[in]      DMA�����ֽ���
  * @retval         ���ؿ�
  */
void MPU6050_UartDMA_Init(uint8_t *rx1_buf, uint8_t *rx2_buf, uint16_t size)
{
    /* -------------- Enable Module Clock Source ----------------------------*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART7, ENABLE);

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART7, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART7, DISABLE);

    GPIO_PinAFConfig(GPIOE, GPIO_PinSource7, GPIO_AF_UART7);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource8, GPIO_AF_UART7);

    /* -------------- Configure GPIO ---------------------------------------*/
    {
        GPIO_InitTypeDef GPIO_InitStructure;
        USART_InitTypeDef USART_InitStructure;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOE, &GPIO_InitStructure);

        USART_DeInit(UART7);

        USART_InitStructure.USART_BaudRate = 921600;  //������Ϊ921600
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_Even; //żУ��
        USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_Init(UART7, &USART_InitStructure);

        USART_DMACmd(UART7, USART_DMAReq_Rx, ENABLE);

        USART_ClearFlag(UART7, USART_FLAG_IDLE);
        USART_ITConfig(UART7, USART_IT_IDLE, ENABLE);

        USART_Cmd(UART7, ENABLE);
    }

    /* -------------- Configure NVIC ---------------------------------------*/
    {
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = UART7_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MPU6050_NVIC;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    }

    {
        DMA_InitTypeDef DMA_InitStructure;
        DMA_DeInit(DMA1_Stream3);//uart7_rx

        DMA_InitStructure.DMA_Channel = DMA_Channel_5;  //ͨ��ѡ��
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (UART7->DR);  //DMA�����ַ
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rx1_buf;            //DMA �洢����ַ
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;               //���赽�洢��ģʽ
        DMA_InitStructure.DMA_BufferSize = size;                              //���ݴ�����
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;      //���������ģʽ
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;               //�洢������ģʽ
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//�������ݳ���:8λ
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;        //�洢�����ݳ���:8λ
        DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;                       // ʹ��ѭ������ģʽ
        DMA_InitStructure.DMA_Priority = DMA_Priority_High;                   //�θ����ȼ�
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;           //�洢��ͻ�����δ��䣬һ�δ���һ���ֽ�
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;   //����ͻ�����δ���
        DMA_Init(DMA1_Stream3, &DMA_InitStructure);

        //����DMA˫������ģʽ
        DMA_DoubleBufferModeConfig(DMA1_Stream3, (uint32_t)rx2_buf, DMA_Memory_0);//�ڴ�0 ��rx1_buf�ȱ�����
        DMA_DoubleBufferModeCmd(DMA1_Stream3, ENABLE);

        DMA_Cmd(DMA1_Stream3, DISABLE); //Add a disable
        DMA_Cmd(DMA1_Stream3, ENABLE);
    }
}

void gyroscope_init(void)
{
    MPU6050_UartDMA_Init(MPU6050_Pack[0].buf,MPU6050_Pack[1].buf,MPU6050_RX_BUF_NUM);
}

const float *get_MPU6050_Angle_point(void)
{
    return INS_Angle;
}

const float *get_MPU6050_Gyro_Point(void)
{
    return INS_gyro;
}

//����7�����ж�
void UART7_IRQHandler(void)
{
    if (USART_GetITStatus(UART7, USART_IT_IDLE) != RESET)
    {
        static uint16_t this_time_rx_len = 0;
			  
        if(DMA_GetCurrentMemoryTarget(DMA1_Stream3) == 0) //�������ֵΪ0��˵��DMA���ڷ��ʻ�����0�������ڿ����жϵĵ�����˵��������0�������Ѿ��ռ���ɣ����Դ�����
        {
            this_time_rx_len = UART7->SR;
            this_time_rx_len = UART7->DR; //���UART_IT_IDLE��־λ
					
					  //��������DMA
            DMA_Cmd(DMA1_Stream3, DISABLE);
					  this_time_rx_len = MPU6050_RX_BUF_NUM - DMA_GetCurrDataCounter(DMA1_Stream3);//�õ����յ����ݸ���
            DMA_SetCurrDataCounter(DMA1_Stream3, MPU6050_RX_BUF_NUM);//����DMA��һ�ν��յ��ֽ���
            DMA1_Stream3->CR |= DMA_SxCR_CT;/* Set Memory 1 as next memory address */       
            DMA_Cmd(DMA1_Stream3, ENABLE);
		        
					  if( this_time_rx_len == GYRO_FRAME_LENGTH)
						{
						    if((MPU6050_Pack[0].data.header==0x55)&&(MPU6050_Pack[0].data.tail==0xAA))
								{
										memcpy(INS_Angle,MPU6050_Pack[0].data.INS_Angle,sizeof(INS_Angle));
										memcpy(INS_gyro,MPU6050_Pack[0].data.INS_gyro,sizeof(INS_gyro));
								}
						}					
        }
        else
        {
					  this_time_rx_len = UART7->SR;
            this_time_rx_len = UART7->DR; //���UART_IT_IDLE��־λ
					
            //��������DMA
            DMA_Cmd(DMA1_Stream3, DISABLE);
					  this_time_rx_len = MPU6050_RX_BUF_NUM - DMA_GetCurrDataCounter(DMA1_Stream3);//�õ����յ����ݸ���
            DMA_SetCurrDataCounter(DMA1_Stream3, MPU6050_RX_BUF_NUM);
            DMA1_Stream6->CR &= ~(DMA_SxCR_CT);/* Set Memory 0 as next memory address */
            DMA_Cmd(DMA1_Stream3, ENABLE);
			      
					  if( this_time_rx_len == GYRO_FRAME_LENGTH)
						{
						    if((MPU6050_Pack[1].data.header==0x55)&&(MPU6050_Pack[1].data.tail==0xAA))
								{
										memcpy(INS_Angle,MPU6050_Pack[1].data.INS_Angle,sizeof(INS_Angle));
										memcpy(INS_gyro,MPU6050_Pack[1].data.INS_gyro,sizeof(INS_gyro));
								}
						}				
        }
    }
}

