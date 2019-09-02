/**
  ****************************(C) COPYRIGHT 2019 IronSprit***********************
  * @file       gimbal_task.c/h
	
  * @brief      �����̨��������.
	
  * @note       ����һ������ֵ����⣬����̨�����������pid.c�����pid_init�����stm32
	              Ӳ�������ж���ѭ��������̨����ĵ��÷��������һ�����ʲ�֪�Ǻ�ԭ��
								
								����취������̨����������дһ��PID��س��򣬵���GIMBAL_PID_Init����
								          ����
  * @history

  *  Version       Date            Author          status
  *  V2.0.0      2019-7-9          Young            ���
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 IronSprit************************
*/

#include "Gimbal_Task.h"
#include "gimbal_behaviour.h"
#include "arm_math.h"
#include "user_lib.h"
#include "Detect_Task.h"
#include "FreeRTOS.h"
#include "task.h"


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
		
		
//��̨���������������
static Gimbal_Control_t     gimbal_move;

//��̨��ʼ��
static void GIMBAL_Init(Gimbal_Control_t  *gimbal_init);
//��̨״̬����
static void GIMBAL_Set_Mode(Gimbal_Control_t *gimbal_set_mode);
//��̨���ݸ���
static void GIMBAL_Feedback_Update(Gimbal_Control_t *gimbal_feedback_update);
//��̨״̬�л���������
static void GIMBAL_Mode_Change_Control_Transit(Gimbal_Control_t *gimbal_mode_change);
//������̨������
static void GIMBAL_Set_Contorl(Gimbal_Control_t *gimbal_set_control);
//��̨����pid����
static void GIMBAL_Control_loop(Gimbal_Control_t *gimbal_control_loop);
//��̨PID�����
static void  gimbal_total_pid_clear(Gimbal_Control_t *gimbal_clear);
//��̨�����е�Ǳ仯ʱ����Ӧ����ԽǶȼ���
static float motor_ecd_to_angle_change(uint16_t ecd, uint16_t offset_ecd);
//�����е������ת��Ϊ���ٶ�
static void MechanicalAngle_ConvertTo_AngularVelocity(Gimbal_Motor_t  *gimbal_motor);
//�Կ��Ƶ�Ŀ��ֵ�������Է��������ԽǶ�
static void GIMBAL_relative_angle_limit(Gimbal_Motor_t *gimbal_motor, float add);
static void GIMBAL_Yaw_absolute_angle_limit(Gimbal_Motor_t *gimbal_motor, float add);
static void GIMBAL_Pitch_absolute_angle_limit(Gimbal_Motor_t *gimbal_motor, float add);
//��ʼ����̨PID
static void GIMBAL_PID_Init(Gimbal_PID_t *pid, uint8_t data_mode, float maxout, float max_iout, float kp, float ki, float kd);
//��̨pid���ݼ���
static float GIMBAL_PID_Calc(Gimbal_PID_t *pid, float fdb, float set);
//��̨pid��������
static void Gimbal_PID_clear(Gimbal_PID_t *gimbal_pid_clear);




//��ʼ��̨������
void gimbal_task(void *pvParameters)
{
    //��ʼ����ʱ
    vTaskDelay(GIMBAL_TASK_INIT_TIME);
    //��̨��ʼ��
    GIMBAL_Init( &gimbal_move);

    //�жϵ���Ƿ�����
    while ( toe_is_error(YawGimbalMotorTOE) || toe_is_error(PitchGimbalMotorTOE) )
    {
        vTaskDelay(GIMBAL_CONTROL_TIME);
    }

    while (1)
    {
        //������̨����ģʽ
        GIMBAL_Set_Mode(&gimbal_move);
        //����ģʽ�л� �������ݹ���
        GIMBAL_Mode_Change_Control_Transit(&gimbal_move);
        //��̨���ݷ���
        GIMBAL_Feedback_Update(&gimbal_move);
        //������̨������
        GIMBAL_Set_Contorl(&gimbal_move);
        //��̨����PID����
        GIMBAL_Control_loop(&gimbal_move);

        if ( gimbal_move.gimbal_mode == GIMBAL_RELAX )
        {
            CAN_CMD_GIMBAL(0,0,NULL,NULL);
        }
        else
        {
            CAN_CMD_GIMBAL(gimbal_move.gimbal_yaw_motor.give_current, gimbal_move.gimbal_pitch_motor.give_current, NULL ,NULL);
        }

        vTaskDelay(GIMBAL_CONTROL_TIME);
    }
}



/*-------------------------------------����YAW������еֵ��Խ�---------------------------------*/


//���ص�����ݰ�ָ��
float  Get_YAWmotor_relative_angle(void)
{
    return  gimbal_move.gimbal_yaw_motor.relative_angle ;
}



/**
  * @brief          ��̨��ʼ��
  * @author         Young
  * @param[in]      ��������ΪGimbal_Control_t�ı���
  * @retval         ��
  * @waring         yaw��pitch�����ʼ��ʱ��е�Ƕȸ���Ϊ0����ֱ�Ӹ���Ӧ����Ļ�е�Ƕȣ��������yaw��������ҿ����ٻ��е�����
  */
static void GIMBAL_Init(Gimbal_Control_t *gimbal_init)
{
    //�������ָ���ȡ
    gimbal_init->gimbal_yaw_motor.gimbal_motor_data   = get_Yaw_Gimbal_Motor_Data_Point();
    gimbal_init->gimbal_pitch_motor.gimbal_motor_data = get_Pitch_Gimbal_Motor_Data_Point();

    //����������ָ���ȡ
    gimbal_init->gimbal_INS_angle_point = get_MPU6050_Angle_point();
    gimbal_init->gimbal_INS_gyro_point  = get_MPU6050_Gyro_Point();

    //ң��������ָ���ȡ
    gimbal_init->gimbal_rc_ctrl = get_remote_control_point();

    //��ʼ����̨ģʽ
    gimbal_init->gimbal_mode = gimbal_init->last_gimbal_mode = GIMBAL_INIT;

    //��ʼ��yaw���pid
    GIMBAL_PID_Init(&gimbal_init->gimbal_yaw_motor.gimbal_absolute_position_pid, DATA_GYRO,  YAW_ABSOLUTE_POSITION_PID_MAX_OUT, YAW_ABSOLUTE_POSITION_PID_MAX_IOUT, YAW_ABSOLUTE_POSITION_PID_KP, YAW_ABSOLUTE_POSITION_PID_KI, YAW_ABSOLUTE_POSITION_PID_KD);
    GIMBAL_PID_Init(&gimbal_init->gimbal_yaw_motor.gimbal_relative_position_pid, DATA_GYRO,  YAW_RELATIVE_POSITION_PID_MAX_OUT, YAW_RELATIVE_POSITION_PID_MAX_IOUT, YAW_RELATIVE_POSITION_PID_KP, YAW_RELATIVE_POSITION_PID_KI, YAW_RELATIVE_POSITION_PID_KD);
    GIMBAL_PID_Init(&gimbal_init->gimbal_yaw_motor.gimbal_absolute_speed_pid,    DATA_NORMAL,YAW_ABSOLUTE_SPEED_PID_MAX_OUT,    YAW_ABSOLUTE_SPEED_PID_MAX_IOUT,    YAW_ABSOLUTE_SPEED_PID_KP,    YAW_ABSOLUTE_SPEED_PID_KI,    YAW_ABSOLUTE_SPEED_PID_KD);
    GIMBAL_PID_Init(&gimbal_init->gimbal_yaw_motor.gimbal_relative_speed_pid,    DATA_NORMAL,YAW_RELATIVE_SPEED_PID_MAX_OUT,    YAW_RELATIVE_SPEED_PID_MAX_IOUT,    YAW_RELATIVE_SPEED_PID_KP,    YAW_RELATIVE_SPEED_PID_KI,    YAW_RELATIVE_SPEED_PID_KD);

    //��ʼ��pitch���pid
    GIMBAL_PID_Init(&gimbal_init->gimbal_pitch_motor.gimbal_absolute_position_pid, DATA_GYRO,   PITCH_ABSOLUTE_POSITION_PID_MAX_OUT, PITCH_ABSOLUTE_POSITION_PID_MAX_IOUT, PITCH_ABSOLUTE_POSITION_PID_KP, PITCH_ABSOLUTE_POSITION_PID_KI, PITCH_ABSOLUTE_POSITION_PID_KD);
    GIMBAL_PID_Init(&gimbal_init->gimbal_pitch_motor.gimbal_relative_position_pid, DATA_GYRO,   PITCH_RELATIVE_POSITION_PID_MAX_OUT, PITCH_RELATIVE_POSITION_PID_MAX_IOUT, PITCH_RELATIVE_POSITION_PID_KP, PITCH_RELATIVE_POSITION_PID_KI, PITCH_RELATIVE_POSITION_PID_KD);
    GIMBAL_PID_Init(&gimbal_init->gimbal_pitch_motor.gimbal_absolute_speed_pid,    DATA_NORMAL, PITCH_ABSOLUTE_SPEED_PID_MAX_OUT,    PITCH_ABSOLUTE_SPEED_PID_MAX_IOUT,    PITCH_ABSOLUTE_SPEED_PID_KP,    PITCH_ABSOLUTE_SPEED_PID_KI,    PITCH_ABSOLUTE_SPEED_PID_KD);
    GIMBAL_PID_Init(&gimbal_init->gimbal_pitch_motor.gimbal_relative_speed_pid,    DATA_NORMAL, PITCH_RELATIVE_SPEED_PID_MAX_OUT,    PITCH_RELATIVE_SPEED_PID_MAX_IOUT,    PITCH_RELATIVE_SPEED_PID_KP,    PITCH_RELATIVE_SPEED_PID_KI,    PITCH_RELATIVE_SPEED_PID_KD);

    //����YAW PITCH�������ƫ��
    gimbal_init->gimbal_yaw_motor.max_relative_angle = MAX_YAW_RANGLE_ANGLE;
    gimbal_init->gimbal_yaw_motor.min_relative_angle = MIN_YAW_RANGLE_ANGLE;

    gimbal_init->gimbal_pitch_motor.max_relative_angle = MAX_PITCH_RANGLE_ANGLE;
    gimbal_init->gimbal_pitch_motor.min_relative_angle = MIN_PITCH_RANGLE_ANGLE;

    //�������PID,����׼��
    gimbal_total_pid_clear(gimbal_init);

    GIMBAL_Feedback_Update(gimbal_init);

    gimbal_init->gimbal_yaw_motor.absolute_angle_set = gimbal_init->gimbal_yaw_motor.absolute_angle;
    gimbal_init->gimbal_yaw_motor.relative_angle_set = 0;
    gimbal_init->gimbal_yaw_motor.absolute_speed_set = gimbal_init->gimbal_yaw_motor.absolute_speed;
    gimbal_init->gimbal_yaw_motor.relative_speed_set = gimbal_init->gimbal_yaw_motor.relative_speed;

    gimbal_init->gimbal_pitch_motor.absolute_angle_set = gimbal_init->gimbal_pitch_motor.absolute_angle;
    gimbal_init->gimbal_pitch_motor.relative_angle_set = 0;
    gimbal_init->gimbal_pitch_motor.absolute_speed_set = gimbal_init->gimbal_pitch_motor.absolute_speed;
    gimbal_init->gimbal_pitch_motor.relative_speed_set = gimbal_init->gimbal_pitch_motor.relative_speed;
}


/*----------------------------��̨���PID����---------------------------------------*/


//��ʼ����̨PID
static void GIMBAL_PID_Init(Gimbal_PID_t *pid, uint8_t data_mode, float maxout, float max_iout, float kp, float ki, float kd)
{
    if (pid == NULL)
    {
        return;
    }
		pid->data_mode = data_mode;
		
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->err[0] = pid->err[1] = pid->fdb = 0.0f;
    pid->Iout = pid->Dout = pid->Pout = pid->out = 0.0f;
		
    pid->max_iout = max_iout;
    pid->max_out = maxout;
}

//��̨pid���ݼ���
static float GIMBAL_PID_Calc(Gimbal_PID_t *pid, float fdb, float set)
{
	  if (pid == NULL)
    {
        return 0.0f;
    }
		
		pid->err[1] = pid->err[0];
		
    pid->fdb = fdb;
    pid->set = set;

    pid->err[0] = set - fdb;
		
    if(pid->data_mode == DATA_GYRO)
		{
		  if( pid->err[0] >  180.0f)  pid->err[0] -= 360.0f; 
		  if( pid->err[0] < -180.0f)  pid->err[0] += 360.0f; 
		}	
		else if(pid->data_mode == DATA_NORMAL)
		{
       //��������
		}
		
    pid->Pout = pid->kp * pid->err[0];
    pid->Iout += pid->ki * pid->err[0];
    pid->Dout = pid->kd * (pid->err[0]-pid->err[1]);
    LimitMax(pid->Iout, pid->max_iout);
    pid->out = pid->Pout + pid->Iout + pid->Dout;
    LimitMax(pid->out, pid->max_out);
    return pid->out;
}

//��̨pid��������
static void Gimbal_PID_clear(Gimbal_PID_t *gimbal_pid_clear)
{
    if (gimbal_pid_clear == NULL)
    {
        return;
    }
    gimbal_pid_clear->err[0] = gimbal_pid_clear->err[1] = gimbal_pid_clear->set = gimbal_pid_clear->fdb = 0.0f;
    gimbal_pid_clear->out = gimbal_pid_clear->Pout = gimbal_pid_clear->Iout = gimbal_pid_clear->Dout = 0.0f;
}

//��̨PID�������������
void  gimbal_total_pid_clear(Gimbal_Control_t *gimbal_clear)
{
    Gimbal_PID_clear(&gimbal_clear->gimbal_yaw_motor.gimbal_absolute_position_pid);
    Gimbal_PID_clear(&gimbal_clear->gimbal_yaw_motor.gimbal_relative_position_pid);
    Gimbal_PID_clear(&gimbal_clear->gimbal_yaw_motor.gimbal_absolute_speed_pid);
    Gimbal_PID_clear(&gimbal_clear->gimbal_yaw_motor.gimbal_relative_speed_pid);

    Gimbal_PID_clear(&gimbal_clear->gimbal_pitch_motor.gimbal_absolute_position_pid);
    Gimbal_PID_clear(&gimbal_clear->gimbal_pitch_motor.gimbal_relative_position_pid);
    Gimbal_PID_clear(&gimbal_clear->gimbal_pitch_motor.gimbal_absolute_speed_pid);
    Gimbal_PID_clear(&gimbal_clear->gimbal_pitch_motor.gimbal_relative_speed_pid);
}


/*----------------------------��̨���PID����---------------------------------------*/


//��̨ģʽ�趨
static void GIMBAL_Set_Mode(Gimbal_Control_t  *gimbal_set_mode)
{
    if (gimbal_set_mode == NULL)
    {
        return;
    }
    gimbal_behaviour_mode_set(gimbal_set_mode);
}



//��̨���ݸ���
static void GIMBAL_Feedback_Update(Gimbal_Control_t  *gimbal_feedback_update)
{
    if (gimbal_feedback_update == NULL)
    {
        return;
    }

    //��̨���ݸ���
    gimbal_feedback_update->gimbal_pitch_motor.absolute_angle = *(gimbal_feedback_update->gimbal_INS_angle_point + 1);
    gimbal_feedback_update->gimbal_pitch_motor.relative_angle = motor_ecd_to_angle_change(gimbal_feedback_update->gimbal_pitch_motor.gimbal_motor_data->ecd, PITCH_OFFSET_ECD );
    gimbal_feedback_update->gimbal_pitch_motor.absolute_speed = *(gimbal_feedback_update->gimbal_INS_gyro_point + 1);
    MechanicalAngle_ConvertTo_AngularVelocity(&gimbal_feedback_update->gimbal_pitch_motor);

    gimbal_feedback_update->gimbal_yaw_motor.absolute_angle = *(gimbal_feedback_update->gimbal_INS_angle_point);
    gimbal_feedback_update->gimbal_yaw_motor.relative_angle = motor_ecd_to_angle_change(gimbal_feedback_update->gimbal_yaw_motor.gimbal_motor_data->ecd, YAW_OFFSET_ECD );
    gimbal_feedback_update->gimbal_yaw_motor.absolute_speed = *(gimbal_feedback_update->gimbal_INS_gyro_point);
    MechanicalAngle_ConvertTo_AngularVelocity(&gimbal_feedback_update->gimbal_yaw_motor);
}



/**
  * @brief          ��̨�����е����ֵ�仯ʱ����Ӧ����ԽǶȼ���
  * @author         Young
  * @param[in]      �����е����ֵ
  * @param[in]      �����е����ֵ���趨ƫ��ֵ
  * @retval         ��Ի�е����ֵ�任Ϊ�Ƕ�ֵ
  * @waring         ��̨����ĽǶ�ֵ�Ǹ����趨ƫ��ֵ����ģ�ƫ��ֵ�ǳ�ʼ����̨ʱ��Ŀ��λ��
  */
static float motor_ecd_to_angle_change(uint16_t ecd, uint16_t offset_ecd)
{
    int32_t relative_ecd = ecd - offset_ecd;

    if (relative_ecd > Half_ecd_range)
    {
        relative_ecd -= ecd_range;
    }
    else if (relative_ecd < -Half_ecd_range)
    {
        relative_ecd += ecd_range;
    }

    return relative_ecd * Motor_Ecd_to_Rad;
}

/**
  * @brief          �����е������ת��Ϊ���ٶ�
  * @author         Young
  * @param[in]      ��������ΪGimbal_Motor_t�ı���
  * @retval         ���ؿ�
  * @waring         ��һ���˲�ƽ��ͨ����е�ǶȻ�����ٶ�ֵ
  */
static void MechanicalAngle_ConvertTo_AngularVelocity(Gimbal_Motor_t  *gimbal_motor)
{
	  int16_t  error_Angle = 0;
	  
    error_Angle = gimbal_motor->gimbal_motor_data->ecd - gimbal_motor->gimbal_motor_data->last_ecd;
		if(error_Angle <= -Half_ecd_range)  //�ж��Ƿ����EC60�����е�Ƕȵ���㡣��Ϊ���ÿ���е�һ��״̬��Ӧһ����е�Ƕȣ��û�е�Ƕ�ֵ�̶���
		{   //����������Ҷ�Ӧֵ�ֱ�Ϊ8000��100�����ڶ�ʱ���ڶ̣��������ڣ��������˶���Ȧ��һȦ��ֵΪ8192.
				error_Angle += ecd_range;   //����ǰֵΪ100����һ��ֵΪ8000��error_AngleΪ-7900���������ף�����8191����Ϊ291����Ϊת������ȷ�Ļ�е�Ƕ�ֵ��������㣬ת��1Ȧ����¼ת����Ȧ����1
		}
		else if(error_Angle > Half_ecd_range)
		{
				error_Angle -= ecd_range;
		}
		gimbal_motor->relative_speed =  GIMBAL_ACCEL_RELATIVE_SPEED_NUM * error_Angle + (1-GIMBAL_ACCEL_RELATIVE_SPEED_NUM)*gimbal_motor->relative_speed; 
}




//��̨״̬�л����棬����״̬�л�����
static void GIMBAL_Mode_Change_Control_Transit(Gimbal_Control_t *gimbal_mode_change)
{
    if( gimbal_mode_change == NULL )
    {
        return;
    }
    if( gimbal_mode_change->last_gimbal_mode == gimbal_mode_change->gimbal_mode )
    {
        return;
    }
    //ģʽ�л����ݱ���
    gimbal_mode_change->gimbal_pitch_motor.absolute_angle_set = gimbal_mode_change->gimbal_pitch_motor.absolute_angle ;
    gimbal_mode_change->gimbal_pitch_motor.relative_angle_set = gimbal_mode_change->gimbal_pitch_motor.relative_angle ;
    gimbal_mode_change->gimbal_pitch_motor.absolute_speed_set = gimbal_mode_change->gimbal_pitch_motor.absolute_speed ;
    gimbal_mode_change->gimbal_pitch_motor.relative_speed_set = gimbal_mode_change->gimbal_pitch_motor.relative_speed ;

    gimbal_mode_change->gimbal_yaw_motor.absolute_angle_set = gimbal_mode_change->gimbal_yaw_motor.absolute_angle ;
    gimbal_mode_change->gimbal_yaw_motor.relative_angle_set = gimbal_mode_change->gimbal_yaw_motor.relative_angle ;
    gimbal_mode_change->gimbal_yaw_motor.absolute_speed_set = gimbal_mode_change->gimbal_yaw_motor.absolute_speed ;
    gimbal_mode_change->gimbal_yaw_motor.relative_speed_set = gimbal_mode_change->gimbal_yaw_motor.relative_speed ;

    //PID������������
    gimbal_total_pid_clear(gimbal_mode_change);

    //��¼�ϴ�ģʽ
    gimbal_mode_change->last_gimbal_mode = gimbal_mode_change->gimbal_mode;
}




//��̨����������
static void GIMBAL_Set_Contorl(Gimbal_Control_t  *gimbal_set_control)
{
    if (gimbal_set_control == NULL)
    {
        return;
    }

    float add_yaw_angle   = 0.0f;
    float add_pitch_angle = 0.0f;

    //�Ƕ������趨
    gimbal_behaviour_control_set(&add_yaw_angle, &add_pitch_angle, gimbal_set_control);

    //�Ƕ�������ֵ���޷�
    if( gimbal_set_control->gimbal_mode == GIMBAL_STOP  || gimbal_set_control->gimbal_mode == GIMBAL_ENCONDE ||
            gimbal_set_control->gimbal_mode == GIMBAL_INIT  )
    {
        GIMBAL_relative_angle_limit( &gimbal_set_control->gimbal_yaw_motor, add_yaw_angle);
        GIMBAL_relative_angle_limit( &gimbal_set_control->gimbal_pitch_motor, add_pitch_angle);
    }
    else if( gimbal_set_control->gimbal_mode == GIMBAL_GYRO)
    {
        GIMBAL_Yaw_absolute_angle_limit( &gimbal_set_control->gimbal_yaw_motor, add_yaw_angle);
        GIMBAL_Pitch_absolute_angle_limit( &gimbal_set_control->gimbal_pitch_motor, -add_pitch_angle);//��Ϊ������pitch��pitch������е�Ǳ仯����һ��,����Ҫ�ı��������
    }
}



//������yaw�Ƕȿ���������������
#ifdef USED_GYRO
static void GIMBAL_Yaw_absolute_angle_limit(Gimbal_Motor_t *gimbal_motor, float add)
{
    static float bias_angle;

    if (gimbal_motor == NULL)
    {
        return;
    }
    //���㵱ǰ�������Ƕ�
    bias_angle = RAD_Format(gimbal_motor->absolute_angle_set - gimbal_motor->absolute_angle);

    //��̨��ԽǶ�+ ���Ƕ� + �����Ƕ� ������� ����е�Ƕ�
    if (gimbal_motor->relative_angle + bias_angle + add >  (gimbal_motor->max_relative_angle))
    {
        //�����������е�Ƕȿ��Ʒ���
        if (add > 0.0f)
        {
            add = (gimbal_motor->max_relative_angle) - gimbal_motor->relative_angle - bias_angle;
        }
    }
    else if (gimbal_motor->relative_angle + bias_angle + add < (gimbal_motor->min_relative_angle) )
    {
        if (add < 0.0f)
        {
            add = (gimbal_motor->min_relative_angle)- gimbal_motor->relative_angle - bias_angle;
        }
    }

    gimbal_motor->absolute_angle_set = RAD_Format( (gimbal_motor->absolute_angle_set) + add );
}
#endif


//������pitch�Ƕȿ��������������ƣ���Ϊ������pitch��pitch������е�Ǳ仯����һ��,����Ҫ�ı��������
#ifdef USED_GYRO
static void GIMBAL_Pitch_absolute_angle_limit(Gimbal_Motor_t *gimbal_motor, float add)
{
    static float bias_angle;

    if (gimbal_motor == NULL)
    {
        return;
    }
    //���㵱ǰ�������Ƕ�
    bias_angle = RAD_Format(gimbal_motor->absolute_angle_set - gimbal_motor->absolute_angle);

    //��̨��ԽǶ�+ ���Ƕ� + �����Ƕ� ������� ����е�Ƕ�
    if (-gimbal_motor->relative_angle + bias_angle + add >  (gimbal_motor->max_relative_angle))
    {
        //�����������е�Ƕȿ��Ʒ���
        if (add > 0.0f)
        {
            add = (gimbal_motor->max_relative_angle) + gimbal_motor->relative_angle - bias_angle;
        }
    }
    else if (-gimbal_motor->relative_angle + bias_angle + add < (gimbal_motor->min_relative_angle) )
    {
        if (add < 0.0f)
        {
            add = (gimbal_motor->min_relative_angle)+ gimbal_motor->relative_angle - bias_angle;
        }
    }
    gimbal_motor->absolute_angle_set = RAD_Format( (gimbal_motor->absolute_angle_set) + add );
}
#endif


//�����ǽǶȹ���
#ifdef USED_GYRO
static float RAD_Format(float rad)
{
    if ( rad > Half_rad_range)
    {
        rad -= rad_range;
    }
    else if (rad < (-Half_rad_range))
    {
        rad += rad_range;
    }
    return 	rad;
}
#endif


//��е�ǽǶȿ���������������
static void GIMBAL_relative_angle_limit(Gimbal_Motor_t *gimbal_motor, float add)
{
    if (gimbal_motor == NULL)
    {
        return;
    }
    gimbal_motor->relative_angle_set += add;

    //�Ƿ񳬹���� ��Сֵ
    if (gimbal_motor->relative_angle_set > (gimbal_motor->max_relative_angle) )
    {
        gimbal_motor->relative_angle_set = gimbal_motor->max_relative_angle;
    }
    else if (gimbal_motor->relative_angle_set < (gimbal_motor->min_relative_angle) )
    {
        gimbal_motor->relative_angle_set = gimbal_motor->min_relative_angle;
    }
}



/**
  * @brief          ��̨����״̬ʹ�ò�ͬ����pid
  * @author         Young
  * @param[in]      ��������ΪGimbal_Control_t����
  * @retval         ���ؿ�
  * @waring         λ���⻷���ٶ��ڻ�
                    ��̨pitch�����Ϻ�������Ҫ������һ������ȥ500��Ϊ����PID���Ϊ0ʱ���ܻ�����ס��̨pitch,��ʵ����ƽ�����Ϻ���������ƫ����
  */
static void GIMBAL_Control_loop( Gimbal_Control_t  *gimbal_control_loop )
{
    if (gimbal_control_loop == NULL)
    {
        return;
    }
		
		//yaw��ͬģʽ���ڲ�ͬ�Ŀ��ƺ���
    if ( gimbal_control_loop->gimbal_mode == GIMBAL_INIT || gimbal_control_loop->gimbal_mode == GIMBAL_STOP || gimbal_control_loop->gimbal_mode == GIMBAL_ENCONDE )
    {

        /*YAW��*/
				gimbal_control_loop->gimbal_yaw_motor.relative_speed_set = GIMBAL_PID_Calc( &gimbal_control_loop->gimbal_yaw_motor.gimbal_relative_position_pid,
								gimbal_control_loop->gimbal_yaw_motor.relative_angle,
								gimbal_control_loop->gimbal_yaw_motor.relative_angle_set ) ;

				gimbal_control_loop->gimbal_yaw_motor.give_current = GIMBAL_PID_Calc( &gimbal_control_loop->gimbal_yaw_motor.gimbal_relative_speed_pid,
								gimbal_control_loop->gimbal_yaw_motor.relative_speed,
								gimbal_control_loop->gimbal_yaw_motor.relative_speed_set);

				/*PITCH��*/
				gimbal_control_loop->gimbal_pitch_motor.relative_speed_set = GIMBAL_PID_Calc( &gimbal_control_loop->gimbal_pitch_motor.gimbal_relative_position_pid,
								gimbal_control_loop->gimbal_pitch_motor.relative_angle,
								gimbal_control_loop->gimbal_pitch_motor.relative_angle_set ) ;

				gimbal_control_loop->gimbal_pitch_motor.give_current = GIMBAL_PID_Calc( &gimbal_control_loop->gimbal_pitch_motor.gimbal_relative_speed_pid,
								gimbal_control_loop->gimbal_pitch_motor.relative_speed,
								gimbal_control_loop->gimbal_pitch_motor.relative_speed_set );  

    }
    else if ( gimbal_control_loop->gimbal_mode == GIMBAL_GYRO)
    {
        /*YAW��*/
        gimbal_control_loop->gimbal_yaw_motor.absolute_speed_set = GIMBAL_PID_Calc( &gimbal_control_loop->gimbal_yaw_motor.gimbal_absolute_position_pid,
                gimbal_control_loop->gimbal_yaw_motor.absolute_angle,
                gimbal_control_loop->gimbal_yaw_motor.absolute_angle_set ) ;

        gimbal_control_loop->gimbal_yaw_motor.give_current = GIMBAL_PID_Calc( &gimbal_control_loop->gimbal_yaw_motor.gimbal_absolute_speed_pid,
                gimbal_control_loop->gimbal_yaw_motor.absolute_speed,
                gimbal_control_loop->gimbal_yaw_motor.absolute_speed_set) ;
          

  			/*PITCH��*/
        gimbal_control_loop->gimbal_pitch_motor.absolute_speed_set = GIMBAL_PID_Calc( &gimbal_control_loop->gimbal_pitch_motor.gimbal_absolute_position_pid,
                gimbal_control_loop->gimbal_pitch_motor.absolute_angle,
                gimbal_control_loop->gimbal_pitch_motor.absolute_angle_set ) ;

        gimbal_control_loop->gimbal_pitch_motor.give_current = -GIMBAL_PID_Calc( &gimbal_control_loop->gimbal_pitch_motor.gimbal_absolute_speed_pid,
                gimbal_control_loop->gimbal_pitch_motor.absolute_speed,
                gimbal_control_loop->gimbal_pitch_motor.absolute_speed_set)-500;   //��Ϊ������pitch��pitch������е�Ǳ仯����һ��,����Ҫ�ı��������   
    }
}


