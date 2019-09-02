/**
  ****************************(C) COPYRIGHT 2019 IronSprit***********************
  * @project    ����2015��Ĺٷ�������еƽ̨�ϴRM������A�ͱ�д�ĳ���ʵ�ֲ���
	              ��������̨�����Ĺ���

  * @brief                     

  * @note       ���⣺������yaw��ֵһֱƮ              
	              
								����취�������ǰ����ϵĳ���Ҫ�ȵ��̰����ϵĳ������ܣ������Ƕ�ֵ�����ȶ���
								         �������ǳ�ʼ������ǰ��2s����ʱ���ȴ������ǰ����ϵĳ����ʼ���ɹ�
												 ����ֵ�ȶ�����ȥ�������ǵ�ֵ�������������Ǿ����ȶ���
								
  * @history
  *  Version       Date            Author          status
  *  V2.0.0      2019-7-14         Young            ���
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 IronSprit************************
*/

#include "stm32f4xx.h"
#include "buzzer.h"
#include "can.h"
#include "delay.h"
#include "led.h"
#include "sys.h"
#include "FreeRTOS.h"
#include "task.h"
#include "remote_control.h"
#include "start_task.h"
#include "gyroscope.h"

void Program_Init(void)
{
	  //����ϵͳ�ж����ȼ�����4
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	
	  //��ʼ����ʱ�������δ�ʱ�ӣ�
   delay_init(configTICK_RATE_HZ);

    //��ʼ��LED	
    LED_init();	
   
	  //��������ʼ��
    buzzer_init(30000-1, 90-1);
	  
    //�����ǳ�ʼ��
    delay_ms(2000);	
	  gyroscope_init();
	
	  //CAN�ӿڳ�ʼ��
    CAN1_mode_init(CAN_SJW_1tq, CAN_BS2_2tq, CAN_BS1_6tq, 5, CAN_Mode_Normal);
	  CAN2_mode_init(CAN_SJW_1tq, CAN_BS2_2tq, CAN_BS1_6tq, 5, CAN_Mode_Normal);
	
	  //ң������ʼ��
    remote_control_init();
}

int main(void)
{
    Program_Init();
	  delay_ms(100);
	  Create_MyTask();
	  vTaskStartScheduler();          //�����������
    while(1);
}
