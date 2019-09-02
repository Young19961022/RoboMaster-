#ifndef DETECT_Task_H
#define DETECT_Task_H

#include "main.h"

#define DETECT_TASK_INIT_TIME    57
#define DETECT_CONTROL_TIME      10

//��������������л�����
#define DETECT_LED_R_ON()       led_red_on()
#define DETECT_LED_R_OFF()      led_red_off()
#define DETECT_LED_R_TOGGLE()   led_red_toggle()

//��ˮ��������
#define DETECT_FLOW_LED_ON(i)   flow_led_on(i)
#define DETECT_FLOW_LED_OFF(i)  flow_led_off(i)

//�������Լ���Ӧ�豸˳��
enum errorList
{
    DBUSTOE = 0,
    YawGimbalMotorTOE,
    PitchGimbalMotorTOE,
    TriggerMotorTOE,
    ChassisMotor1TOE,   //4
    ChassisMotor2TOE,
    ChassisMotor3TOE,
    ChassisMotor4TOE,   //7

    errorListLength,
};

typedef __packed struct
{
    uint32_t newTime;                 //���¼�¼ʱ��
    uint32_t lastTime;                //�ϴμ�¼ʱ��
    uint32_t Losttime;                //����ʱ��
    uint32_t worktime;                //����ʱ��
    uint16_t setOfflineTime : 12;     //��������ʱ����ֵ
    uint16_t setOnlineTime  : 12;     //��������ʱ����ֵ
    uint8_t enable : 1;               //�豸���ʹ�ܰ�ť
    uint8_t Priority : 4;             //�豸���ȼ�
    uint8_t isLost : 1;               //�豸����״̬
    float frequency;                   //�豸����Ƶ��

} error_t;

void   detect_task(void *pvParameters);      //����������
uint8_t toe_is_error(uint8_t err);           //���ض�Ӧ���豸�Ƿ�����
const  error_t *getErrorListPoint(void);    //�����豸�б��ַ
void   DetectHook(uint8_t toe);             //���߹��Ӻ���
void DetectInit(uint32_t time);   //��ʼ�������б�
void DetectDisplay(void);         //��ʾ������Ϣ

#endif
