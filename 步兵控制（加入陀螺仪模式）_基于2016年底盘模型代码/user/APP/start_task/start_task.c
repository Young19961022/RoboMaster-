/**
  ****************************(C) COPYRIGHT 2019 IronSprit***********************
  * @file       start_task.c/h

  * @brief      �������񣬽�����������������Դ�������������ȼ�               

  * @note       �ٽ����ܱ���һ�δ��벻������������жϴ��
									 
  * @history
  *  Version       Date            Author          status
  *  V1.0.0       2019-6-14        Young            ���
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 IronSprit************************
*/

#include "start_task.h"
#include "Detect_Task.h"
#include "User_Task.h"
#include "chassis_task.h"
#include "Gimbal_Task.h"

static TaskHandle_t StartTask_Handler; //������
static TaskHandle_t ChassisTask_Handler;
static TaskHandle_t DetectTask_Handler;
static TaskHandle_t UserTask_Handler;
static TaskHandle_t GimbalTask_Handler;

/*--------�������Լ�������-------*/
void Create_MyTask(void)
{
    //�Զ�̬����������ʼ����
    xTaskCreate((TaskFunction_t )start_task,            //������
                (const char*    )"start_task",          //��������
                (uint16_t       )START_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���(һ���ǲ��õ�)
                (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&StartTask_Handler);   //������(�Ƚ���Ҫ�����񴴽��ɹ��Ժ�᷵�ش������������)
}

/*--------��ʼ�����ʵ��-------*/
void start_task(void *pvParameters)//void* ���۲�����ʲô���͵ģ����ܴ���
{
    taskENTER_CRITICAL();    //�����ٽ���

    xTaskCreate((TaskFunction_t)gimbal_task,
                (const char *)"GimbalTask",
                (uint16_t)Gimbal_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)Gimbal_TASK_PRIO,
                (TaskHandle_t *)&GimbalTask_Handler);	
								
	  xTaskCreate((TaskFunction_t)chassis_task,
                (const char *)"ChassisTask",
                (uint16_t)Chassis_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)Chassis_TASK_PRIO,
                (TaskHandle_t *)&ChassisTask_Handler);							

    xTaskCreate((TaskFunction_t)detect_task,
                (const char *)"DetectTask",
                (uint16_t)Detect_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)Detect_TASK_PRIO,
                (TaskHandle_t *)&DetectTask_Handler);								

    xTaskCreate((TaskFunction_t)user_task,
                (const char *)"UserTask",
                (uint16_t)User_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)User_TASK_PRIO,
                (TaskHandle_t *)&UserTask_Handler);								

    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
}

