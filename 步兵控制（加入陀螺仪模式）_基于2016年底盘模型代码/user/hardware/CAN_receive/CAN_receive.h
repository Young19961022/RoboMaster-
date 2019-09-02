#ifndef _CANTASK_H
#define _CANTASK_H

#include "main.h"
#include "stm32f4xx.h"

#define CHASSIS_CAN   CAN1                                                                   
#define GIMBAL_CAN    CAN2

/* ���巢�ͺͷ��ر��ĵ�ID�� */
typedef enum
{
	  //���ͱ���ID
    CAN_CHASSIS_LOW_ID  = 0x200, 
	  CAN_CHASSIS_HIGH_ID = 0x1FF,
	  CAN_GIMBAL_ALL_ID   = 0x1FF,
	
	  //���ձ���ID
    CAN_EC60_M1_ID = 0x206,  //�˵��ʹ����̨����ĵ��
    CAN_EC60_M2_ID = 0x202,
    CAN_EC60_M3_ID = 0x203,
    CAN_EC60_M4_ID = 0x204,
	
	  CAN_YAW_MOTOR_ID     = 0x205,
    CAN_PITCH_MOTOR_ID   = 0x206,
	
} can_msg_id_e;


//������ݽṹ��
typedef struct
{
    uint16_t  ecd;            //��е�Ƕ�
    int16_t  actual_current;  //ʵ�ʵ���
    int16_t  given_current;   //��������
    uint8_t  hall_value;      //��������ֵ
    int16_t  last_ecd;
	
} motor_data_t;

//���͵��̿�������
void CAN_CMD_CHASSIS(uint32_t SendID, int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4);
//������̨��������
void CAN_CMD_GIMBAL(int16_t yaw, int16_t pitch,int16_t rev1, int16_t rev2);

//���ص��̵��������ַ��ͨ��ָ�뷽ʽ��ȡԭʼ����,i�ķ�Χ��0-3����Ӧ0x201-0x204,
const motor_data_t *get_Chassis_Motor_Data_Point(uint8_t i);
//����yaw���������ַ��ͨ��ָ�뷽ʽ��ȡԭʼ����
const motor_data_t *get_Yaw_Gimbal_Motor_Data_Point(void);
//����pitch���������ַ��ͨ��ָ�뷽ʽ��ȡԭʼ����
const motor_data_t *get_Pitch_Gimbal_Motor_Data_Point(void);

//CAN���մ�����		
static void CAN_hook(CanRxMsg *rx_message);

#endif
