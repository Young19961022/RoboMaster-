#ifndef PID_H
#define PID_H

#include "main.h"

enum PID_MODE
{
    PID_POSITION = 0,
    PID_DELTA
};

enum DATA_MODE
{
    DATA_GYRO = 0,
    DATA_NORMAL,
};


typedef struct
{
    uint8_t  mode;
	  uint8_t  data_mode;
    //PID ������
    float Kp;
    float Ki;
    float Kd;

    float max_out;  //������
    float max_iout; //���������

    float set;
    float fdb;

    float out;
    float Pout;
    float Iout;
    float Dout;
    float Dbuf[3];  //΢���� 0���� 1��һ�� 2���ϴ�
    float error[3]; //����� 0���� 1��һ�� 2���ϴ�

} PidTypeDef;


extern void PID_Init(PidTypeDef *pid, uint8_t mode, uint8_t data_mode, const float PID[3], float max_out, float max_iout); //PID��ʼ��
extern float PID_Calc(PidTypeDef *pid, float ref, float set);  //PID����
extern void PID_clear(PidTypeDef *pid);   //PID���

#endif
