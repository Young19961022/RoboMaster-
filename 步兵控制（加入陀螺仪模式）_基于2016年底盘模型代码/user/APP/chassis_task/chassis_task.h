#ifndef CHASSISTASK_H
#define CHASSISTASK_H

#include "arm_math.h"
#include "CAN_Receive.h"
#include "pid.h"
#include "Remote_Control.h"
#include "user_lib.h"
#include "Gimbal_Task.h"

//����ʼ����ʱ����
#define CHASSIS_TASK_INIT_TIME    357
//����������Ƽ��  ms
#define CHASSIS_CONTROL_TIME_MS    2
//����������Ƽ��   s
#define CHASSIS_CONTROL_TIME_S    0.002


//���ҵ�ң����ͨ������
#define CHASSIS_Y_CHANNEL           2
//ǰ���ң����ͨ������
#define CHASSIS_X_CHANNEL           3
//��ת��ң����ͨ������
#define CHASSIS_Z_CHANNEL           0
//ң����ģʽͨ������
#define CHASSIS_MODE_SWITCH         0


//ǰ�����Ҽ��̰�������
#define CHASSIS_FRONT_KEY     KEY_PRESSED_OFFSET_W
#define CHASSIS_BACK_KEY      KEY_PRESSED_OFFSET_S
#define CHASSIS_LEFT_KEY      KEY_PRESSED_OFFSET_A
#define CHASSIS_RIGHT_KEY     KEY_PRESSED_OFFSET_D
//������ת����
#define CHASSIS_TURNLEFT_KEY      KEY_PRESSED_OFFSET_Q
#define CHASSIS_TURNRIGHT_KEY     KEY_PRESSED_OFFSET_E

//�����������
#define CHASSIS_STRAIGHT_KEY       KEY_PRESSED_OFFSET_F
//���幦�ܿ��ư���
#define  CHASSIS_SPEEDDOWN_KEY     KEY_PRESSED_OFFSET_CTRL
//������ٰ���
#define  CHASSIS_SPEEDUP_KEY       KEY_PRESSED_OFFSET_SHIFT


//���̵������ٶ�
#define MAX_WHEEL_SPEED               3.5f
//�����˶��������ǰ���ٶ�(m/s)
#define NORMAL_MAX_CHASSIS_SPEED_X    2.5f
//�����˶��������ƽ���ٶ�(m/s)
#define NORMAL_MAX_CHASSIS_SPEED_Y    2.3f
//�����˶����������ת�ٶ�(��/s)
#define NORMAL_MAX_CHASSIS_SPEED_Z  180.0f


//һ���˲������趨
//���ձ������е��˲�����д�����궨��ֵԽС��������Խ�󣬵�ƽ���Խ��ͣ�ֻ���ڿɽ��ܵ������ȷ�Χ��ȡ�þ����ܺõ�ƽ�ȶ�
#define CHASSIS_ACCEL_X_NUM        0.1666666667f
#define CHASSIS_ACCEL_Y_NUM        0.3333333333f
#define CHASSIS_ACCEL_Z_NUM        0.1666666667f

//ң����ǰ��ҡ�ˣ�max 660��ת���ɳ���ǰ���ٶȣ�m/s���ı���
#define CHASSIS_VX_RC_SEN    0.0009f
//ң��������ҡ�ˣ�max 660��ת���ɳ��������ٶȣ�m/s���ı���
#define CHASSIS_VY_RC_SEN    0.0009f
//ң������תҡ�ˣ�max 660��ת���ɳ�����ת�ٶȣ�m/s���ı���
#define CHASSIS_VZ_RC_SEN    0.0016f

//EC60ת�٣�r/min��ת���ɵ����ٶ�(m/s)�ı���
#define CHASSIS_MOTOR_RPM_TO_VECTOR_SEN    0.007984881329f

//ң������������
#define CHASSIS_RC_DEADLINE        10

#define MOTOR_SPEED_TO_CHASSIS_SPEED_VX   0.25f
#define MOTOR_SPEED_TO_CHASSIS_SPEED_VY   0.25f
#define MOTOR_SPEED_TO_CHASSIS_SPEED_WZ   0.25f

//������̨��װ������Ȩ(��ת�ٶ�,����ǰ�������ֲ�ͬ�趨�ٶȵı�����Ȩ) ��0Ϊ����̨���ڳ����ģ�����Ҫ������
#define CHASSIS_WZ_SET_SCALE       0.1f

//���ó��������־�
#define MOTOR_DISTANCE_TO_CENTER   0.145f

//�趨����Σ��ֵ
#define   chassis_power_danger      20.0f
//����������ƹ���
#define   chassis_standard_power    80.0f

//���̵���ٶ�PID
#define EC60_MOTOR_SPEED_PID_KP          10000.0f
#define EC60_MOTOR_SPEED_PID_KI          120.0f
#define EC60_MOTOR_SPEED_PID_KD          3000.0f
#define EC60_MOTOR_SPEED_PID_MAX_OUT     15000.0f
#define EC60_MOTOR_SPEED_PID_MAX_IOUT    4000.0f

//���̸���λ��PID
#define CHASSIS_FOLLOW_GIMBAL_PID_KP          0.035f
#define CHASSIS_FOLLOW_GIMBAL_PID_KI           0.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_KD           2.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_MAX_OUT    200.0f
#define CHASSIS_FOLLOW_GIMBAL_PID_MAX_IOUT     0.0f


//����״̬���趨
typedef enum
{
    CHASSIS_RELAX,
    CHASSIS_STOP,
    CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW,
    CHASSIS_VECTOR_NO_FOLLOW_YAW,

} chassis_mode_e;


//�������Ϣ���ݰ�
typedef struct
{
    const motor_data_t   *chassis_motor_data;
    float speed;
    float speed_set;
    int16_t give_current;

} Chassis_Motor_t;


typedef struct
{
    uint8_t  data_mode;
	
	  float Kp;
    float Ki;
    float Kd;

    float set;
    float fdb;
    float error[2];

    float max_out;
    float max_iout;

    float Pout;
    float Iout;
    float Dout;

    float out;
} Chassis_PID_t;


typedef  struct
{
    const RC_ctrl_t         *chassis_RC;          //����ʹ�õ�ң����ָ��
    //const Referee_System_t  *chassis_referee;     //���̲���ϵͳ����

    chassis_mode_e         chassis_mode;          //���̿���״̬��
    chassis_mode_e         last_chassis_mode;     //�����ϴο���״̬��

    Chassis_Motor_t        motor_chassis[4];      //���̵��������
    Chassis_PID_t          motor_speed_pid[4];    //���̵���ٶ�pid
    Chassis_PID_t          chassis_follow_pid;    //���̸���Ƕ�pid

    first_order_filter_type_t     chassis_cmd_slow_set_vx;    //����ǰ��һ���˲�
    first_order_filter_type_t     chassis_cmd_slow_set_vy;    //��������һ���˲�
    first_order_filter_type_t     chassis_cmd_slow_set_vz;    //������תһ���˲�

    float vx;                         //�����ٶ� ǰ������ ǰΪ����      ��λ m/s
    float vy;                         //�����ٶ� ���ҷ��� ��Ϊ��       ��λ m/s
    float wz;                         //���̽��ٶȣ���ʱ��Ϊ��           ��λ ��/s

    float vx_set;                     //�����趨�ٶ� ǰ������ ǰΪ����  ��λ  m/s
    float vy_set;                     //�����趨�ٶ� ���ҷ��� ��Ϊ����  ��λ  m/s
    float wz_set;                     //���̽��ٶȣ�  ��ʱ��Ϊ��         ��λ  ��/s

    float last_vx_set;                //�����趨�ٶ� ǰ������ ǰΪ����  ��λ  m/s
    float last_vy_set;                //�����趨�ٶ� ���ҷ��� ��Ϊ����  ��λ  m/s
    float last_wz_set;                //���̽��ٶȣ�  ��ʱ��Ϊ��         ��λ  ��/s


    float  error_aobut_gimbalYAW;        //���������̨���

} chassis_move_t;



void chassis_task(void *pvParameters);
uint8_t get_chassis_mode(void);
void chassis_rc_to_control_vector(float *vx_set, float *vy_set, chassis_move_t *chassis_move_rc_to_vector);
float CHASSIS_PID_Calc(Chassis_PID_t *pid, float fdb, float set);

#endif
