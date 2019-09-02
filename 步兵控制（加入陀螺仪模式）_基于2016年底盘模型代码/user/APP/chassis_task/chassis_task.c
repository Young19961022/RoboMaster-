/**
  ****************************(C) COPYRIGHT 2019 IronSprit***********************
  * @file       chassis.c/h

  * @brief      ��ɵ��̿�������

  * @note       1��ö�ٿ���ֱ�ӵ��궨��ʹ�ã��ҵ�һ�μ����ֲ������÷�
	              2���������ֵ��̵����˶�ѧģ��ʵ��ȫ���ƶ�
								3�����ձ������е��˲�����д�����궨��ֵԽС��������Խ�󣬵�ƽ����
								   ���ͣ�ֻ���ڿɽ��ܵ������ȷ�Χ��ȡ�þ����ܺõ�ƽ�ȶ�

  * @history

  *  Version       Date            Author          status
  *  V2.0.0       2019-6-16        Young            ���
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 IronSprit************************
*/

#include "chassis_task.h"
#include "chassis_behaviour.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Detect_Task.h"
#include "stdlib.h"


#define LimitMax(input, max)   \
    {                          \
        if (input > max)       \
        {                      \
            input = max;       \
        }                      \
        else if (input < -max) \
        {                      \
            input = -max;      \
        }                      \
    }
		
//��������
#define rc_deadline_limit(input, output, dealine)        \
    {                                                    \
        if ((input) > (dealine) || (input) < -(dealine)) \
        {                                                \
            (output) = (input);                          \
        }                                                \
        else                                             \
        {                                                \
            (output) = 0;                                \
        }                                                \
    }



//�����˶�����
static chassis_move_t   chassis_move;

//���̳�ʼ��
static void chassis_init(chassis_move_t *chassis_move_init);
//����״̬��ѡ��
static void chassis_set_mode(chassis_move_t *chassis_move_mode);
//�������ݸ���
static void chassis_feedback_update(chassis_move_t *chassis_move_update);
//����״̬�ı����
void chassis_mode_change_control_transit(chassis_move_t *chassis_move_transit);
//�������ø���ң����������
static void chassis_set_contorl(chassis_move_t *chassis_move_control);
//����PID�����Լ��˶��ֽ�
static void chassis_control_loop(chassis_move_t *chassis_move_control_loop);

//�����ٶȷ���
static void chassis_vector_to_mecanum_wheel_speed(const float vx_set, const float vy_set, const float wz_set, float wheel_speed[4]);
//����PID��غ���
static void CHASSIS_PID_Init(Chassis_PID_t *pid, uint8_t data_mode, float maxout, float max_iout, float kp, float ki, float kd);
float CHASSIS_PID_Calc(Chassis_PID_t *pid, float fdb, float set);
static void CHASSIS_PID_clear(Chassis_PID_t *chassis_pid_clear);

/*------------------------------------����������-------------------------------------*/


void chassis_task(void *pvParameters)
{
    //����һ��ʱ��
    vTaskDelay(CHASSIS_TASK_INIT_TIME);

    //���̳�ʼ��
    chassis_init(&chassis_move);

    //�жϵ��̵���Ƿ�����
    while (toe_is_error(ChassisMotor1TOE) || toe_is_error(ChassisMotor2TOE) || toe_is_error(ChassisMotor3TOE) || toe_is_error(ChassisMotor4TOE) || toe_is_error(DBUSTOE))
    {
        vTaskDelay(CHASSIS_CONTROL_TIME_MS);
    }

    while (1)
    {
        //ң��������״̬
        chassis_set_mode(&chassis_move);
        //ң����״̬�л����ݱ���
        chassis_mode_change_control_transit(&chassis_move);
        //�������ݸ���
        chassis_feedback_update(&chassis_move);
        //���̿���������
        chassis_set_contorl(&chassis_move);
        //���̿���PID����
        chassis_control_loop(&chassis_move);

        CAN_CMD_CHASSIS(CAN_CHASSIS_LOW_ID,0, chassis_move.motor_chassis[1].give_current,chassis_move.motor_chassis[2].give_current, chassis_move.motor_chassis[3].give_current);
        CAN_CMD_CHASSIS(CAN_CHASSIS_HIGH_ID,0,chassis_move.motor_chassis[0].give_current,0,0);

        //ϵͳ��ʱ
        vTaskDelay(CHASSIS_CONTROL_TIME_MS);

    }
}



/*----------------------------���̵��PID����---------------------------------------*/


//��ʼ������PID
static void CHASSIS_PID_Init(Chassis_PID_t *pid, uint8_t data_mode, float maxout, float max_iout, float kp, float ki, float kd)
{
    if (pid == NULL)
    {
        return;
    }
		pid->data_mode = data_mode;
		
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;

    pid->error[0] = pid->error[1] = pid->fdb = 0.0f;
    pid->Iout = pid->Dout = pid->Pout = pid->out = 0.0f;
		
    pid->max_iout = max_iout;
    pid->max_out = maxout;
}

//��̨pid���ݼ���
float CHASSIS_PID_Calc(Chassis_PID_t *pid, float fdb, float set)
{
	  if (pid == NULL)
    {
        return 0.0f;
    }
		
		pid->error[1] = pid->error[0];
		
    pid->fdb = fdb;
    pid->set = set;

    pid->error[0] = set - fdb;
		
    if(pid->data_mode == DATA_GYRO)
		{
		  if( pid->error[0] >  180.0f)  pid->error[0] -= 360.0f; 
		  if( pid->error[0] < -180.0f)  pid->error[0] += 360.0f; 
		}	
		else if(pid->data_mode == DATA_NORMAL)
		{
       //��������
		}
		
    pid->Pout = pid->Kp * pid->error[0];
    pid->Iout += pid->Ki * pid->error[0];
    pid->Dout = pid->Kd * (pid->error[0]-pid->error[1]);
    LimitMax(pid->Iout, pid->max_iout);
    pid->out = pid->Pout + pid->Iout + pid->Dout;
    LimitMax(pid->out, pid->max_out);
    return pid->out;
}

//��̨pid��������
static void CHASSIS_PID_clear(Chassis_PID_t *chassis_pid_clear)
{
    if (chassis_pid_clear == NULL)
    {
        return;
    }
    chassis_pid_clear->error[0] = chassis_pid_clear->error[1] = chassis_pid_clear->set = chassis_pid_clear->fdb = 0.0f;
    chassis_pid_clear->out = chassis_pid_clear->Pout = chassis_pid_clear->Iout = chassis_pid_clear->Dout = 0.0f;
}



/*--------------------------------���ص���״̬��-----------------------------------*/

 uint8_t get_chassis_mode(void)
{
   if( chassis_move.chassis_mode == CHASSIS_VECTOR_NO_FOLLOW_YAW ) 
	 {
	   return 1;
	 } 
	 else
	 {
	   return 0;
	 }
}

/*-----------------------------------���̳�ʼ��------------------------------------------*/


static void chassis_init(chassis_move_t  *chassis_move_init)
{
    if (chassis_move_init == NULL)
    {
        return;
    }

    //һ���˲�����ֵ
    const static float chassis_x_order_filter[1] = {CHASSIS_ACCEL_X_NUM};
    const static float chassis_y_order_filter[1] = {CHASSIS_ACCEL_Y_NUM};

    //���̿���״̬Ϊֹͣ
    chassis_move_init->chassis_mode = CHASSIS_STOP;

    //��ȡң����ָ��
    chassis_move_init->chassis_RC = get_remote_control_point();

    //��ʼ�����̵��PID
    for ( uint8_t i = 0; i < 4; i++)
    {
        chassis_move_init->motor_chassis[i].chassis_motor_data = get_Chassis_Motor_Data_Point(i);
        CHASSIS_PID_Init(&chassis_move_init->motor_speed_pid[i], DATA_NORMAL, EC60_MOTOR_SPEED_PID_MAX_OUT, EC60_MOTOR_SPEED_PID_MAX_IOUT,EC60_MOTOR_SPEED_PID_KP,EC60_MOTOR_SPEED_PID_KI,EC60_MOTOR_SPEED_PID_KD);
    }
    //��ʼ�����̸�����̨PID
		CHASSIS_PID_Init(&chassis_move_init->chassis_follow_pid, DATA_GYRO, CHASSIS_FOLLOW_GIMBAL_PID_MAX_OUT, CHASSIS_FOLLOW_GIMBAL_PID_MAX_IOUT,CHASSIS_FOLLOW_GIMBAL_PID_KP,CHASSIS_FOLLOW_GIMBAL_PID_KI,CHASSIS_FOLLOW_GIMBAL_PID_KD);
		
    //��һ���˲�����б����������
    first_order_filter_init(&chassis_move_init->chassis_cmd_slow_set_vx, CHASSIS_CONTROL_TIME_S, chassis_x_order_filter);
    first_order_filter_init(&chassis_move_init->chassis_cmd_slow_set_vy, CHASSIS_CONTROL_TIME_S, chassis_y_order_filter);

    //����һ������
    chassis_feedback_update(chassis_move_init);
}



/*---------------------------------����ģʽ�趨----------------------------------------------*/


/*ģʽ�趨*/
static void chassis_set_mode(chassis_move_t *chassis_move_mode)
{
    if (chassis_move_mode == NULL)
    {
        return;
    }

    chassis_behaviour_mode_set(chassis_move_mode);
}



/*--------------------------------ģʽ�л����ݴ���------------------------------------------*/


void chassis_mode_change_control_transit(chassis_move_t  *chassis_move_transit)
{
    if (chassis_move_transit == NULL)
    {
        return;
    }

    if (chassis_move_transit->last_chassis_mode == chassis_move_transit->chassis_mode)
    {
        return;
    }

    //���̸���PID����
    CHASSIS_PID_clear(&chassis_move_transit->chassis_follow_pid);

    //�ض���������
    chassis_move_transit->vx_set = 0;
    chassis_move_transit->vy_set = 0;
    chassis_move_transit->wz_set = 0;

    chassis_move_transit->last_chassis_mode = chassis_move_transit->chassis_mode;
}


/*---------------------------------------�������ݸ���-----------------------------------*/



static void chassis_feedback_update(chassis_move_t *chassis_move_update)
{
    int16_t  error_Angle = 0;

    if (chassis_move_update == NULL)
    {
        return;
    }

    for (uint8_t i = 0; i < 4; i++)
    {
        //���µ���ٶ�
        error_Angle = chassis_move_update->motor_chassis[i].chassis_motor_data->ecd - chassis_move_update->motor_chassis[i].chassis_motor_data->last_ecd;
        if(error_Angle <= -4096)  //�ж��Ƿ����EC60�����е�Ƕȵ���㡣��Ϊ���ÿ���е�һ��״̬��Ӧһ����е�Ƕȣ��û�е�Ƕ�ֵ�̶���
        {   //����������Ҷ�Ӧֵ�ֱ�Ϊ8000��100�����ڶ�ʱ���ڶ̣��������ڣ��������˶���Ȧ��һȦ��ֵΪ8192.
            error_Angle += 8191;   //����ǰֵΪ100����һ��ֵΪ8000��error_AngleΪ-7900���������ף�����8192����Ϊ292����Ϊת������ȷ�Ļ�е�Ƕ�ֵ��������㣬ת��1Ȧ����¼ת����Ȧ����1
        }
        else if(error_Angle > 4096)
        {
            error_Angle -= 8191;
        }
        chassis_move_update->motor_chassis[i].speed = CHASSIS_MOTOR_RPM_TO_VECTOR_SEN * (error_Angle+chassis_move_update->motor_chassis[i].speed/2);
    }

    //���µ���ǰ���ٶ� x�� ƽ���ٶ�y����ת�ٶ�wz������ϵΪ����ϵ
    chassis_move_update->vx = (-chassis_move_update->motor_chassis[0].speed + chassis_move_update->motor_chassis[1].speed + chassis_move_update->motor_chassis[2].speed - chassis_move_update->motor_chassis[3].speed) * MOTOR_SPEED_TO_CHASSIS_SPEED_VX;
    chassis_move_update->vy = (-chassis_move_update->motor_chassis[0].speed - chassis_move_update->motor_chassis[1].speed + chassis_move_update->motor_chassis[2].speed + chassis_move_update->motor_chassis[3].speed) * MOTOR_SPEED_TO_CHASSIS_SPEED_VY;
    chassis_move_update->wz = (-chassis_move_update->motor_chassis[0].speed - chassis_move_update->motor_chassis[1].speed - chassis_move_update->motor_chassis[2].speed - chassis_move_update->motor_chassis[3].speed) * MOTOR_SPEED_TO_CHASSIS_SPEED_WZ / MOTOR_DISTANCE_TO_CENTER;
    
		//���������̨���Ƕ�
		chassis_move_update->error_aobut_gimbalYAW = Get_YAWmotor_relative_angle();
		
    chassis_move_update->last_vx_set = chassis_move_update->vx_set;
    chassis_move_update->last_vy_set = chassis_move_update->vy_set;
    chassis_move_update->last_wz_set = chassis_move_update->wz_set;
}



/*---------------------------------------���̿���������-------------------------------------*/


//�������������
static void chassis_set_contorl(chassis_move_t  *chassis_move_control)
{
    if (chassis_move_control == NULL)
    {
        return;
    }

    //�����ٶ�
    float vx_set = 0.0f, vy_set = 0.0f, wz_set = 0.0f;

    chassis_behaviour_control_set(&vx_set, &vy_set, &wz_set, chassis_move_control);

    //������̨ģʽ
    if (chassis_move_control->chassis_mode == CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW)
    {
        /*����̨��������������ϵ1���Ե��̳�������������ϵ2��ң����������̨ת������vx_set��vy_set�ǻ�����̨����ϵ�ģ������vx_set��vy_set
			    �ķ�����ֵ�нǣ���������,�ټ�����̨�����֮��ļнǣ�֮�����Ӧ������������ϵ�Ϸֽ��vx��vy���ٶȣ���ʹ����ûת����������Ҳ������̨����б����*/
			  static float Gimbal_V = 0.0f, delta_angle = 0.0f;
			
				arm_sqrt_f32(vx_set*vx_set+vy_set*vy_set,&Gimbal_V);
			  delta_angle = chassis_move_control->error_aobut_gimbalYAW/57.3f + atan2(vy_set,vx_set);
			
				chassis_move_control->vx_set = arm_cos_f32(delta_angle)*Gimbal_V;
				chassis_move_control->vy_set = arm_sin_f32(delta_angle)*Gimbal_V;
				chassis_move_control->wz_set = wz_set;
    }
    //��������̨ģʽ
    else if (chassis_move_control->chassis_mode == CHASSIS_VECTOR_NO_FOLLOW_YAW || chassis_move_control->chassis_mode ==  CHASSIS_STOP || chassis_move_control->chassis_mode ==  CHASSIS_RELAX )
    {
        //����ʵ�ʵ��̵��ٶ��趨
        chassis_move_control->vx_set = vx_set;
        chassis_move_control->vy_set = vy_set;
        chassis_move_control->wz_set = wz_set;
    }

    //�ٶ��޷�
    chassis_move_control->vx_set = float_constrain(chassis_move_control->vx_set, - NORMAL_MAX_CHASSIS_SPEED_X, NORMAL_MAX_CHASSIS_SPEED_X );
    chassis_move_control->vy_set = float_constrain(chassis_move_control->vy_set, - NORMAL_MAX_CHASSIS_SPEED_Y, NORMAL_MAX_CHASSIS_SPEED_Y );
    chassis_move_control->wz_set = float_constrain(chassis_move_control->wz_set, - NORMAL_MAX_CHASSIS_SPEED_Z, NORMAL_MAX_CHASSIS_SPEED_Z );
}






//ң���������ݴ���vx�ٶȣ�vy��vw�ٶ�
void chassis_rc_to_control_vector(float *vx_set, float *vy_set, chassis_move_t *chassis_move_rc_to_vector)
{
    if (chassis_move_rc_to_vector == NULL || vx_set == NULL || vy_set == NULL)
    {
        return;
    }

    //ң����ԭʼͨ��ֵ
    int16_t vx_channel, vy_channel;
    float vx_set_channel, vy_set_channel;

    //��������,��Ϊң�������ܴ��ڲ��� ҡ�����м䣬��ֵ��Ϊ0
    rc_deadline_limit(chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_X_CHANNEL], vx_channel, CHASSIS_RC_DEADLINE);
    rc_deadline_limit(chassis_move_rc_to_vector->chassis_RC->rc.ch[CHASSIS_Y_CHANNEL], vy_channel, CHASSIS_RC_DEADLINE);

    //ת��Ϊ����ǰ���ٶ�
    vx_set_channel = vx_channel * (-CHASSIS_VX_RC_SEN);
    vy_set_channel = vy_channel * (-CHASSIS_VY_RC_SEN);

    //���̿��Ʋ���ǰ�������ƶ�
    if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_FRONT_KEY)
    {
        vx_set_channel =   -NORMAL_MAX_CHASSIS_SPEED_X*0.55f;
    }
    else if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_BACK_KEY)
    {
        vx_set_channel =   NORMAL_MAX_CHASSIS_SPEED_X*0.55f;
    }
    if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_LEFT_KEY)
    {
        vy_set_channel =   NORMAL_MAX_CHASSIS_SPEED_Y*0.55f;
    }
    else if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_RIGHT_KEY)
    {
        vy_set_channel = - NORMAL_MAX_CHASSIS_SPEED_Y*0.55f;
    }

    //SHIFT����
    if( chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_SPEEDUP_KEY)
    {
        vx_set_channel = vx_set_channel*1.6f;
        vy_set_channel = vy_set_channel*1.6f;
    }
    //ctrl����
    else if( chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_SPEEDDOWN_KEY)
    {
        vx_set_channel = vx_set_channel*0.6f;
        vy_set_channel = vy_set_channel*0.6f;
    }

    //һ�׵�ͨ�˲�����б����Ϊ�����ٶ�����
		if(chassis_move_rc_to_vector->chassis_RC->key.v) //ʹ�ü���ʱ������һ���˲�
		{
		    first_order_filter_cali(&chassis_move_rc_to_vector->chassis_cmd_slow_set_vx, vx_set_channel);
				first_order_filter_cali(&chassis_move_rc_to_vector->chassis_cmd_slow_set_vy, vy_set_channel);
		}
    else //ʹ��ң����ʱ����Ϊң������������ģ����������һ���˲����ԣ�������һ���˲�
		{
		    chassis_move_rc_to_vector->chassis_cmd_slow_set_vx.out = vx_set_channel;
			  chassis_move_rc_to_vector->chassis_cmd_slow_set_vy.out = vy_set_channel;
		}

    //��ʹ�ü��̻�����ֹͣ�źţ�����Ҫ�������٣�ֱ�Ӽ��ٵ���
    if (vx_set_channel < CHASSIS_RC_DEADLINE * CHASSIS_VX_RC_SEN && vx_set_channel > -CHASSIS_RC_DEADLINE * CHASSIS_VX_RC_SEN)
    {
        chassis_move_rc_to_vector->chassis_cmd_slow_set_vx.out = 0.0f;
    }
    if (vy_set_channel < CHASSIS_RC_DEADLINE * CHASSIS_VY_RC_SEN && vy_set_channel > -CHASSIS_RC_DEADLINE * CHASSIS_VY_RC_SEN)
    {
        chassis_move_rc_to_vector->chassis_cmd_slow_set_vy.out = 0.0f;
    }   
		
    *vx_set = chassis_move_rc_to_vector->chassis_cmd_slow_set_vx.out;
    *vy_set = chassis_move_rc_to_vector->chassis_cmd_slow_set_vy.out;
}



/*--------------------------------------------���̿������ݼ���----------------------------------------------*/



//���̿��Ƽ���
static void chassis_control_loop(chassis_move_t *chassis_move_control_loop)
{
    float max_vector = 0.0f, vector_rate = 0.0f;
    float temp = 0.0f;

    float wheel_speed[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    //�����˶��ٶȷֽ�
    chassis_vector_to_mecanum_wheel_speed(chassis_move_control_loop->vx_set,
                                          chassis_move_control_loop->vy_set, chassis_move_control_loop->wz_set, wheel_speed);


    //��ֵ���ӿ����ٶȣ�������������ٶ�
    for (uint8_t i = 0; i < 4; i++)
    {
        chassis_move_control_loop->motor_chassis[i].speed_set = wheel_speed[i];

        temp = fabs(chassis_move_control_loop->motor_chassis[i].speed_set);
        if (max_vector < temp)   max_vector = temp;
    }

    if ( max_vector > MAX_WHEEL_SPEED )
    {
        vector_rate = MAX_WHEEL_SPEED / max_vector;
        for (uint8_t i = 0; i < 4; i++)
        {
            chassis_move_control_loop->motor_chassis[i].speed_set *= vector_rate;//�ȱ������ٸ����ٶ�
        }
    }

    //����pid
    for (uint8_t i = 0; i < 4; i++)
    {
        CHASSIS_PID_Calc(&chassis_move_control_loop->motor_speed_pid[i], chassis_move_control_loop->motor_chassis[i].speed, chassis_move_control_loop->motor_chassis[i].speed_set);
    }

    //��ֵ����ֵ
    for (uint8_t i = 0; i < 4; i++)
    {
        chassis_move_control_loop->motor_chassis[i].give_current = (int16_t)(chassis_move_control_loop->motor_speed_pid[i].out);
    }

}




//�����ٶȷֽ�
//�˷�����O-���������ֵ��̵����˶�ѧģ�ͣ�ʵ��ȫ���ƶ�����
//���˶�ѧģ�ͣ�inverse kinematic model���õ��Ĺ�ʽ���ǿ��Ը��ݵ��̵��˶�״̬������ĸ����ӵ��ٶ�
static void chassis_vector_to_mecanum_wheel_speed(const float vx_set, const float vy_set, const float wz_set, float wheel_speed[4])
{
    //��ת��ʱ�� ������̨��ǰ��������ǰ������ 0,1 ��ת���ٶȱ����� �������� 2,3 ��ת���ٶȱ��
    wheel_speed[0] = -vx_set - vy_set + (  CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
    wheel_speed[1] = vx_set - vy_set  + (  CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
    wheel_speed[2] = vx_set + vy_set  + (- CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
    wheel_speed[3] = -vx_set + vy_set + (- CHASSIS_WZ_SET_SCALE - 1.0f) * MOTOR_DISTANCE_TO_CENTER * wz_set;
}

