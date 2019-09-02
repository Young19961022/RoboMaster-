/**
  ****************************(C) COPYRIGHT 2019 IronSprit***********************
  * @file       remote_control.c/h

  * @brief      ң��������ң������ͨ������DBUS��Э�鴫�䣬���ô���1�����ж���DMA˫
	              ���������䷽ʽ��ԼCPU��Դ��ͬʱʵʱ���DBUS�Ƿ����ߡ�               

  * @note       1����������ͨ�����ڿ����ж�����������freeRTOS����
	*             2����static�����ⲿ����,��ı������ӷ�ʽ��ʹ��ֻ�ڱ��ļ��ڲ���Ч��
	                 �������ļ��������ӻ����øñ�����
	* 					  3��ʹ��static���ں�������ʱ���Ժ��������ӷ�ʽ����Ӱ�죬ʹ�ú���
								   ֻ�ڱ��ļ��ڲ���Ч���������ļ��ǲ��ɼ��ġ����õ����������ļ���
									 ͬ�������������ţ�����Ҳ�ǶԺ��������һ�ֱ������ơ�
	*							4��ע�⿴����1�����ж������ʱ��ֵ�洢��0��1
	              5��typedef __packed struct�Ǳ���1�ֽڶ��룬���ṹ��Ĵ�С�ɳ�Ա����
								   ����ʵֵ����
									 
  * @history
  *  Version       Date            Author          status
  *  V2.0.0       2019-6-14        Young            ���
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 IronSprit************************
*/

#include "remote_control.h"
#include "Detect_Task.h"
#include "stm32f4xx.h"

//ң�������Ʊ���
static RC_ctrl_t  rc_ctrl;
//����ԭʼ���ݣ�Ϊ18���ֽڣ�����36���ֽڳ��ȣ���ֹDMA����Խ��
static uint8_t   SBUS_rx_buf[2][SBUS_RX_BUF_NUM];

void RC_init(uint8_t *rx1_buf, uint8_t *rx2_buf, uint16_t size)
{
    /* -------------- Enable Module Clock Source ----------------------------*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_DMA2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART1, DISABLE);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1); //PB7  usart1 rx
    /* -------------- Configure GPIO ---------------------------------------*/
    {
        GPIO_InitTypeDef GPIO_InitStructure;
        USART_InitTypeDef USART_InitStructure;
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOB, &GPIO_InitStructure);

        USART_DeInit(USART1);

        USART_InitStructure.USART_BaudRate = 100000;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_Even;  //żУ��
        USART_InitStructure.USART_Mode = USART_Mode_Rx;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_Init(USART1, &USART_InitStructure);

        USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

        USART_ClearFlag(USART1, USART_FLAG_IDLE);
        USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);

        USART_Cmd(USART1, ENABLE);
    }

    /* -------------- Configure NVIC ---------------------------------------*/
    {
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = RC_NVIC;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
    }

    /* -------------- Configure DMA -----------------------------------------*/
    {
        DMA_InitTypeDef DMA_InitStructure;
        DMA_DeInit(DMA2_Stream2);

        DMA_InitStructure.DMA_Channel = DMA_Channel_4;   //ͨ��ѡ��
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (USART1->DR); //DMA�����ַ
        DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rx1_buf;            //DMA �洢����ַ
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;               //���赽�洢��ģʽ
        DMA_InitStructure.DMA_BufferSize = size;                              //���ݴ�����
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;      //���������ģʽ
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;               //�洢������ģʽ
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//�������ݳ���:8λ
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;       //�洢�����ݳ���:8λ
        DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;// ʹ��ѭ������ģʽ
        DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;//������ȼ�
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;//�洢��ͻ�����δ��䣬һ�δ���һ���ֽ�
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//����ͻ�����δ���
        DMA_Init(DMA2_Stream2, &DMA_InitStructure);
			
			  //����DMA˫������ģʽ
        DMA_DoubleBufferModeConfig(DMA2_Stream2, (uint32_t)rx2_buf, DMA_Memory_0);//�ڴ�0 ��rx1_buf�ȱ�����
        DMA_DoubleBufferModeCmd(DMA2_Stream2, ENABLE);
				
        DMA_Cmd(DMA2_Stream2, DISABLE); //Add a disable
        DMA_Cmd(DMA2_Stream2, ENABLE);
    }
}

//����ң�������Ʊ�����ͨ��ָ�봫�ݷ�ʽ������Ϣ
const RC_ctrl_t* get_remote_control_point(void)
{
    return &rc_ctrl;
}

//ң�������ճ�ʼ��
void remote_control_init(void)
{
    RC_init(SBUS_rx_buf[0], SBUS_rx_buf[1], SBUS_RX_BUF_NUM);
}

//���ݴ�����
//��DMA���յ�ԭʼ����ת��Ϊң������Ҫʹ�õ�����
static void DBUS_TO_RC(volatile const uint8_t *sbus_buf, RC_ctrl_t *rc_ctrl)
{
    if (sbus_buf == NULL || rc_ctrl == NULL)
    {
        return;
    }

    rc_ctrl->rc.ch[0] = (sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff;        //!< Channel 0
    rc_ctrl->rc.ch[1] = ((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff; //!< Channel 1
    rc_ctrl->rc.ch[2] = ((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) |          //!< Channel 2
                         (sbus_buf[4] << 10)) & 0x07ff;
    rc_ctrl->rc.ch[3] = ((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff; //!< Channel 3
    rc_ctrl->rc.s[0] = ((sbus_buf[5] >> 4) & 0x0003);                       //!< Switch left
    rc_ctrl->rc.s[1] = ((sbus_buf[5] >> 4) & 0x000C) >> 2;                  //!< Switch right
    rc_ctrl->mouse.x = sbus_buf[6] | (sbus_buf[7] << 8);                    //!< Mouse X axis
    rc_ctrl->mouse.y = sbus_buf[8] | (sbus_buf[9] << 8);                    //!< Mouse Y axis
    rc_ctrl->mouse.z = sbus_buf[10] | (sbus_buf[11] << 8);                  //!< Mouse Z axis
    rc_ctrl->mouse.press_l = sbus_buf[12];                                  //!< Mouse Left Is Press ?
    rc_ctrl->mouse.press_r = sbus_buf[13];                                  //!< Mouse Right Is Press ?
    rc_ctrl->key.v = sbus_buf[14] | (sbus_buf[15] << 8);                    //!< KeyBoard value
    rc_ctrl->rc.ch[4] = sbus_buf[16] | (sbus_buf[17] << 8);                 //NULL

    rc_ctrl->rc.ch[0] -= RC_CH_VALUE_OFFSET; //��ȥ�м�ֵ1024
    rc_ctrl->rc.ch[1] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[2] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[3] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[4] -= RC_CH_VALUE_OFFSET;
}

//����1�����жϷ�����
//����DMA˫����ģʽ���ֱ���ң�������ݣ�ͬʱ���DBUS�Ƿ����
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        static uint16_t this_time_rx_len = 0;

        if(DMA_GetCurrentMemoryTarget(DMA2_Stream2) == 0) //�������ֵΪ0��˵��DMA���ڷ��ʻ�����0�������ڿ����жϵĵ�����˵��������0�������Ѿ��ռ���ɣ����Դ�����
        {
            this_time_rx_len = USART1->SR;
            this_time_rx_len = USART1->DR; //���USART_IT_IDLE��־λ
					
					  //��������DMA
            DMA_Cmd(DMA2_Stream2, DISABLE);
            this_time_rx_len = SBUS_RX_BUF_NUM - DMA_GetCurrDataCounter(DMA2_Stream2);//�õ����յ����ݸ���
            DMA_SetCurrDataCounter(DMA2_Stream2, SBUS_RX_BUF_NUM);//����DMA��һ�ν��յ��ֽ���
            DMA2_Stream2->CR |= DMA_SxCR_CT;/* Set Memory 1 as next memory address */
            DMA_Cmd(DMA2_Stream2, ENABLE);
            if(this_time_rx_len == RC_FRAME_LENGTH)
            {
                //����ң��������
                DBUS_TO_RC(SBUS_rx_buf[0], &rc_ctrl);
                //��¼���ݽ���ʱ��
                DetectHook(DBUSTOE);
            }
        }
        else
        {
					  this_time_rx_len = USART1->SR;
            this_time_rx_len = USART1->DR; //���USART_IT_IDLE��־λ
					
            //��������DMA
            DMA_Cmd(DMA2_Stream2, DISABLE);
            this_time_rx_len = SBUS_RX_BUF_NUM - DMA_GetCurrDataCounter(DMA2_Stream2);
            DMA_SetCurrDataCounter(DMA2_Stream2, SBUS_RX_BUF_NUM);
            DMA2_Stream2->CR &= ~(DMA_SxCR_CT);/* Set Memory 0 as next memory address */
            DMA_Cmd(DMA2_Stream2, ENABLE);
            if(this_time_rx_len == RC_FRAME_LENGTH)
            {
                //����ң��������
                DBUS_TO_RC(SBUS_rx_buf[1], &rc_ctrl);
                //��¼���ݽ���ʱ��
                DetectHook(DBUSTOE);
            }
        }
    }
}

