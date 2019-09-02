#ifndef _GIMBAL_TASK_H
#define _GIMBAL_TASK_H

#include "main.h"
#include "CAN_Receive.h"
#include "chassis_task.h"
#include "pid.h"
#include "remote_control.h"
#include "gyroscope.h"

#define USED_GYRO

//�����ʼ����ʱʱ��
#define  GIMBAL_TASK_INIT_TIME    201
//��̨��������
#define  GIMBAL_CONTROL_TIME       2

//YAW ,PITCH����ͨ���Լ�״̬����ͨ��
#define  YAW_CHANNEL               0
#define  PITCH_CHANNEL             1
#define  MODEL_CHANNEL             0

//ң���������趨
#define  GIMBAL_RC_DEADLINE        10

//YAW ,PITCH�Ƕ��������ȣ���ң�����������
#define  YAW_RC_SEN             -0.0007f
#define  PITCH_RC_SEN           -0.0007f

//YAW,PITCH�Ƕ��������ȣ����������ı���
#define  YAW_MOUSE_SEN          0.0045f
#define  PITCH_MOUSE_SEN        0.00585f

//�������ֵ����Լ���ֵ
#define  Half_ecd_range           4096
#define  ecd_range                8191

//�����ǽǶ�ֵ���ֵ
#define  Half_rad_range            180
#define  rad_range                 360

//����PITCH,YAW����Ƕȼ���(��)
#define  MAX_YAW_RANGLE_ANGLE      80.0f
#define  MIN_YAW_RANGLE_ANGLE     -90.0f

#define  MAX_PITCH_RANGLE_ANGLE    15.0f
#define  MIN_PITCH_RANGLE_ANGLE   -15.0f

//�������ֵ(0~8191)ת���ɽǶ�ֵ(-180~180)     360/8192
#define  Motor_Ecd_to_Rad       0.0439453125f

//����PITCH,YAW���������ֵ
#define  PITCH_OFFSET_ECD      7010
#define  YAW_OFFSET_ECD        3720


//��̨��ʼ������ֵ����������,��������Χ��ֹͣһ��ʱ���Լ����ʱ��6s������ʼ��״̬��
#define GIMBAL_INIT_ANGLE_ERROR      0.1f
#define GIMBAL_INIT_STOP_TIME_MS      100
#define GIMBAL_INIT_TIME_MS          1500

//��̨��ʼ������ֵ���ٶ��Լ����Ƶ��ĽǶ�
#define GIMBAL_INIT_PITCH_SPEED  0.004f
#define GIMBAL_INIT_YAW_SPEED    0.008f

//��̨��ʼ�����ԽǶ�λ��
#define   INIT_YAW_SET       0.0f
#define   INIT_PITCH_SET     0.0f

//һ���˲������趨
//���ձ��ļ��е��˲�����д�����궨��ֵԽС��������ԽС����ƽ������ߣ�ֻ���ڿɽ��ܵ������ȷ�Χ��ȡ�þ����ܺõ�ƽ�ȶ�
#define GIMBAL_ACCEL_RELATIVE_SPEED_NUM     0.1666666667f


/*-------------------��̨��������Ǿ��Խ�PID---------------------*/

//pitch �ٶȻ� PID�����Լ� PID���������������
#define PITCH_ABSOLUTE_SPEED_PID_KP            40.0f
#define PITCH_ABSOLUTE_SPEED_PID_KI             0.8f
#define PITCH_ABSOLUTE_SPEED_PID_KD             0.0f
#define PITCH_ABSOLUTE_SPEED_PID_MAX_OUT    13000.0f
#define PITCH_ABSOLUTE_SPEED_PID_MAX_IOUT    1000.0f

//yaw �ٶȻ� PID�����Լ� PID���������������
#define YAW_ABSOLUTE_SPEED_PID_KP              35.0f
#define YAW_ABSOLUTE_SPEED_PID_KI               0.8f
#define YAW_ABSOLUTE_SPEED_PID_KD               0.0f
#define YAW_ABSOLUTE_SPEED_PID_MAX_OUT      13000.0f
#define YAW_ABSOLUTE_SPEED_PID_MAX_IOUT      1000.0f

//pitch �ǶȻ� �Ƕ��������ǽ��� PID�����Լ� PID���������������
#define PITCH_ABSOLUTE_POSITION_PID_KP           12.0f
#define PITCH_ABSOLUTE_POSITION_PID_KI            0.0f
#define PITCH_ABSOLUTE_POSITION_PID_KD           50.0f
#define PITCH_ABSOLUTE_POSITION_PID_MAX_OUT     400.0f
#define PITCH_ABSOLUTE_POSITION_PID_MAX_IOUT      0.0f

//yaw �ǶȻ� �Ƕ��������ǽ��� PID�����Լ� PID���������������
#define YAW_ABSOLUTE_POSITION_PID_KP             5.0f
#define YAW_ABSOLUTE_POSITION_PID_KI              0.0f
#define YAW_ABSOLUTE_POSITION_PID_KD            20.0f
#define YAW_ABSOLUTE_POSITION_PID_MAX_OUT       600.0f
#define YAW_ABSOLUTE_POSITION_PID_MAX_IOUT        0.0f



/*-------------------��̨�����е����Խ�PID---------------------*/

//pitch �ٶȻ� PID�����Լ� PID���������������
#define PITCH_RELATIVE_SPEED_PID_KP          800.0f
#define PITCH_RELATIVE_SPEED_PID_KI           10.0f
#define PITCH_RELATIVE_SPEED_PID_KD           0.0f
#define PITCH_RELATIVE_SPEED_PID_MAX_OUT  10000.0f
#define PITCH_RELATIVE_SPEED_PID_MAX_IOUT  200.0f

//yaw �ٶȻ� PID�����Լ� PID���������������
#define YAW_RELATIVE_SPEED_PID_KP            500.0f
#define YAW_RELATIVE_SPEED_PID_KI             30.0f
#define YAW_RELATIVE_SPEED_PID_KD            0.0f
#define YAW_RELATIVE_SPEED_PID_MAX_OUT    10000.0f
#define YAW_RELATIVE_SPEED_PID_MAX_IOUT    3000.0f

//pitch �ǶȻ� �Ƕ��ɱ����� PID�����Լ� PID���������������
#define PITCH_RELATIVE_POSITION_PID_KP         1.0f
#define PITCH_RELATIVE_POSITION_PID_KI          0.0f
#define PITCH_RELATIVE_POSITION_PID_KD          2.0f
#define PITCH_RELATIVE_POSITION_PID_MAX_OUT   400.0f
#define PITCH_RELATIVE_POSITION_PID_MAX_IOUT    0.0f

//yaw �ǶȻ� �Ƕ��ɱ����� PID�����Լ� PID���������������
#define YAW_RELATIVE_POSITION_PID_KP           0.15f
#define YAW_RELATIVE_POSITION_PID_KI           0.0f
#define YAW_RELATIVE_POSITION_PID_KD           2.5f
#define YAW_RELATIVE_POSITION_PID_MAX_OUT     400.0f
#define YAW_RELATIVE_POSITION_PID_MAX_IOUT      0.0f


typedef enum
{
    GIMBAL_INIT = 0,   //��ʼ��ģʽ
    GIMBAL_RELAX,      //ʧ��ģʽ
    GIMBAL_STOP,       //ֹͣģʽ
    GIMBAL_GYRO,       //�����ǿ���ģʽ
    GIMBAL_KEY_TO_ALIGN,//һ������ģʽ
    GIMBAL_ENCONDE,    //��е�ǿ���ģʽ

} gimbal_motor_mode_e;


typedef struct
{
    uint8_t  data_mode;
	
	  float kp;
    float ki;
    float kd;

    float set;
    float fdb;
    float err[2];

    float max_out;
    float max_iout;

    float Pout;
    float Iout;
    float Dout;

    float out;
} Gimbal_PID_t;


typedef  struct
{
    const motor_data_t   *gimbal_motor_data;    //�����������

    Gimbal_PID_t    gimbal_relative_position_pid;    //��е��λ��PID
    Gimbal_PID_t    gimbal_absolute_position_pid;    //������λ��PID
    Gimbal_PID_t    gimbal_relative_speed_pid;       //��е���ٶ�PID
    Gimbal_PID_t    gimbal_absolute_speed_pid;       //�������ٶ�PID

    float relative_angle;       //��ǰ��е��Խ�  
    float relative_angle_set;   //��ǰ��е��Խ��趨

    float absolute_angle;       //��ǰ�����Ǿ��Խǣ�-180 ~ 180 �ȣ�
    float absolute_angle_set;   //��ǰ�����Ǿ��Խ��趨

    float relative_speed;        //��ǰ��еת�٣���/s��
    float relative_speed_set;    //��ǰ��еת���趨����/s��

    float absolute_speed;        //��ǰ������ת�٣���/s��
    float absolute_speed_set;    //��ǰ������ת���趨����/s��

    float max_relative_angle;    //��е��������ƽǶ�
    float min_relative_angle;    //��е����С���ƽǶ�

    int16_t  give_current;

} Gimbal_Motor_t;


typedef  struct
{
    gimbal_motor_mode_e    gimbal_mode;        //��̨��ǰģʽ
    gimbal_motor_mode_e    last_gimbal_mode;   //��̨�ϴ�ģʽ

    const RC_ctrl_t       *gimbal_rc_ctrl;          //��̨ң����ָ��

    const float            *gimbal_INS_angle_point;  //�����ǽǶ�����ָ��
    const float            *gimbal_INS_gyro_point;   //�����ǽ��ٶ�����ָ��

    Gimbal_Motor_t      gimbal_yaw_motor;       //YAW�������ݰ�
    Gimbal_Motor_t      gimbal_pitch_motor;     //pitch�������ݰ�

} Gimbal_Control_t;


void gimbal_task(void *pvParameters);
float  Get_YAWmotor_relative_angle(void);
static float RAD_Format(float rad);
#endif

