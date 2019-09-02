/**
  ****************************(C) COPYRIGHT 2019 IronSprit***********************
  * @file       detect_task.c/h
  * @brief      �豸�����ж�����ͨ��freeRTOS�δ�ʱ����Ϊϵͳʱ�䣬�豸��ȡ���ݺ�
  *             ����DetectHook��¼��Ӧ�豸��ʱ�䣬�ڸ������ͨ���жϼ�¼ʱ����ϵͳ
  *             ʱ��֮�����жϵ��ߣ�ͬʱ����ߵ����ȼ�������ͨ��LED�ķ�ʽ�ı䣬����
  *             �˸���ˮ����ʾSBUBң������������̨�ϵĵ����4�����̵��������Ҳͨ��
  *             �����˸��������ʾ�����롣
  * @history
  *  Version       Date            Author          status
  *  V1.0.0      2019-6-16          DJI             ���
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 IronSprit************************
*/

#include "Detect_Task.h"
#include "led.h"
#include "buzzer.h"
#include "CAN_Receive.h"
#include "Remote_Control.h"
#include "FreeRTOS.h"
#include "task.h"

//�������豸�ṹ��
static error_t    errorList[errorListLength + 1];

//�����ж�����
void detect_task(void *pvParameters)
{
    static  uint32_t  systemTime;
    systemTime = xTaskGetTickCount();//���ڻ�ȡϵͳ��ǰ���е�ʱ�ӽ�����

    //��ʼ��
    DetectInit(systemTime);

    //����һ��ʱ��
    vTaskDelay(DETECT_TASK_INIT_TIME);

    while (1)
    {
        systemTime = xTaskGetTickCount();

        for (int i = 0; i < errorListLength; i++)
        {
            //δʹ�ܣ������豸���
            if (errorList[i].enable == 0)
            {
                continue;
            }

            //�������ų�
            if (systemTime - errorList[i].worktime < errorList[i].setOnlineTime)
            {
                //�ո����ߣ����ܴ������ݲ��ȶ�
                errorList[i].isLost = 0;
            }
            //��ʱ����
            else if (systemTime - errorList[i].newTime > errorList[i].setOfflineTime)
            {
                if (errorList[i].isLost == 0)
                {
                    //��¼�����Լ�����ʱ��
                    errorList[i].isLost = 1;
                    errorList[i].Losttime = systemTime;
                }
            }
            //��������
            else
            {
                errorList[i].isLost = 0;

                //����Ƶ��
                if (errorList[i].newTime > errorList[i].lastTime)
                {
                    errorList[i].frequency = configTICK_RATE_HZ / (float)(errorList[i].newTime - errorList[i].lastTime);
                }
            }
        }
        DetectDisplay();
        vTaskDelay(DETECT_CONTROL_TIME);
    }
}

//�豸�������ݹ��Ӻ���
void DetectHook(uint8_t toe)
{
    errorList[toe].lastTime = errorList[toe].newTime;
    errorList[toe].newTime  = xTaskGetTickCount();

    //���¶�ʧ���
    if (errorList[toe].isLost)
    {
        errorList[toe].isLost   = 0;
        errorList[toe].worktime = errorList[toe].newTime;
    }
}

//�����豸�б��ַ
const error_t *getErrorListPoint(void)
{
    return errorList;
}

//���ض�Ӧ���豸�Ƿ�ʧ
uint8_t toe_is_error(uint8_t err)
{
    return (errorList[err].isLost == 1);
}

void DetectDisplay(void)
{
    uint8_t i = 0;
    uint8_t error_flag =0;
    static uint16_t detect_counter =0;

    //8����ˮ��ʾ������豸״̬
    for (i = 0; i < errorListLength; i++)
    {
        if (errorList[i].isLost && (errorList[i].enable == 1) )
        {
            DETECT_FLOW_LED_OFF(i);
            error_flag =1;
        }
        else
        {
            DETECT_FLOW_LED_ON(i);
        }
    }

    //ӵ�д�����״̬�����˸
    detect_counter++;
    if(detect_counter == 30 )
    {
        if(error_flag == 1 )
        {
            DETECT_LED_R_TOGGLE();
        }
        else
        {
            DETECT_LED_R_OFF();
        }

        detect_counter =0;
    }

    //��������ʾ����ʾ�д�
    if(error_flag == 1)
    {
        buzzer_on(10,15000);
    }
    else
    {
        buzzer_off();
    }
}

void DetectInit(uint32_t time)
{
    //����ʱ����ֵ ������ʱ����ֵ�����ȼ���ʹ��״̬
    uint16_t setItem[errorListLength][4] =
    {
        {30, 40,  15, 1},   //DBUS
        {2,   3,  14, 1},   //yaw
        {2,   3,  13, 1},   //pitch
        {10, 10,  12, 0},   //trigger
        {10, 10,  11, 1},   //motor1
        {10, 10,  10, 1},   //motor2
        {10, 10,   9, 1},   //motor3
        {10, 10,   8, 1},   //motor4
    };

    for (uint8_t i = 0; i < errorListLength; i++)
    {
        errorList[i].setOfflineTime = setItem[i][0];
        errorList[i].setOnlineTime =  setItem[i][1];
        errorList[i].Priority = setItem[i][2];
        errorList[i].enable = setItem[i][3];

        errorList[i].isLost = 1;
        errorList[i].frequency = 0.0f;
        errorList[i].newTime =  time;
        errorList[i].lastTime = time;
        errorList[i].Losttime = time;
        errorList[i].worktime = time;
    }

}
