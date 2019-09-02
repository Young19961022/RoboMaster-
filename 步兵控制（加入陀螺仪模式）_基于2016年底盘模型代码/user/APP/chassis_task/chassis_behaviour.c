/**
  ****************************(C) COPYRIGHT 2019 IronSprit***********************
  * @file       chassis_behaviour.c/h

  * @brief      ��ɵ�����Ϊ�����

  * @note

  * @history

  *  Version       Date            Author          status
  *  V2.0.0       2019-6-17        Young            ���
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 IronSprit************************
*/

#include "chassis_behaviour.h"
#include "chassis_task.h"
#include "arm_math.h"
#include "Detect_Task.h"

//����������������
static void chassis_zero_force_control(float *vx_can_set, float *vy_can_set, float *wz_set, chassis_move_t *chassis_move_rc_to_vector);
//����ֹͣ��������
static void chassis_no_move_control(float *vx_set, float *vy_set, float *wz_set, chassis_move_t *chassis_move_rc_to_vector);
//���̳��������������
static void chassis_follow_gimbal_yaw_control(float *vx_set, float *vy_set, float *wz_set, chassis_move_t *chassis_move_rc_to_vector);
//���������������������
static void chassis_no_follow_control(float *vx_set, float *vy_set, float *wz_set, chassis_move_t *chassis_move_rc_to_vector);


/*----------------------ģʽ�趨��Ϊ��-------------------------------*/


void chassis_behaviour_mode_set(chassis_move_t   *chassis_move_mode)
{
    if (chassis_move_mode == NULL)
    {
        return;
    }

    //����ģʽ
    if ( chassis_move_mode->chassis_mode == CHASSIS_RELAX)
    {
        if(toe_is_error(DBUSTOE)== 0)
        {
            chassis_move_mode->chassis_mode = CHASSIS_STOP;
        }
    }
    //ֹͣģʽ
    else if ( chassis_move_mode->chassis_mode  == CHASSIS_STOP )
    {

        if(toe_is_error(DBUSTOE)== 1)
        {
            chassis_move_mode->chassis_mode = CHASSIS_RELAX;
        }
        else if( switch_is_up(chassis_move_mode->chassis_RC->rc.s[CHASSIS_MODE_SWITCH]) )
        {
            chassis_move_mode->chassis_mode  =  CHASSIS_VECTOR_NO_FOLLOW_YAW ;
        }
        else if( switch_is_down(chassis_move_mode->chassis_RC->rc.s[CHASSIS_MODE_SWITCH]) )
        {
            chassis_move_mode->chassis_mode  =  CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW ;
        }
    }
    //������̨ģʽ
    else if ( chassis_move_mode->chassis_mode  == CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW )
    {
        if(toe_is_error(DBUSTOE)== 1)
        {
            chassis_move_mode->chassis_mode = CHASSIS_RELAX;
        }
        else if( switch_is_mid(chassis_move_mode->chassis_RC->rc.s[CHASSIS_MODE_SWITCH]) )
        {
            chassis_move_mode->chassis_mode  =  CHASSIS_STOP ;
        }
        else if( switch_is_up(chassis_move_mode->chassis_RC->rc.s[CHASSIS_MODE_SWITCH]) )
        {
            chassis_move_mode->chassis_mode  =  CHASSIS_VECTOR_NO_FOLLOW_YAW ;
        }
    }
    //��������̨ģʽ
    else if ( chassis_move_mode->chassis_mode  == CHASSIS_VECTOR_NO_FOLLOW_YAW )
    {
        if(toe_is_error(DBUSTOE)== 1)
        {
            chassis_move_mode->chassis_mode = CHASSIS_RELAX;
        }
        else if( switch_is_mid(chassis_move_mode->chassis_RC->rc.s[CHASSIS_MODE_SWITCH]) )
        {
            chassis_move_mode->chassis_mode  =  CHASSIS_STOP ;
        }
        else if( switch_is_down(chassis_move_mode->chassis_RC->rc.s[CHASSIS_MODE_SWITCH]) )
        {
            chassis_move_mode->chassis_mode  =  CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW ;
        }
    }
}

/*---------------------------------���̶���������Ϊ��-----------------------------------------*/



void chassis_behaviour_control_set(float *vx_set, float *vy_set, float *wz_set, chassis_move_t *chassis_move_rc_to_vector)
{

    if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
    {
        return;
    }

    if ( chassis_move_rc_to_vector->chassis_mode == CHASSIS_RELAX )
    {
        chassis_zero_force_control(vx_set, vy_set, wz_set, chassis_move_rc_to_vector);
    }
    else if ( chassis_move_rc_to_vector->chassis_mode == CHASSIS_STOP )
    {
        chassis_no_move_control(vx_set, vy_set, wz_set, chassis_move_rc_to_vector);
    }
    else if ( chassis_move_rc_to_vector->chassis_mode == CHASSIS_VECTOR_FOLLOW_GIMBAL_YAW )
    {
        chassis_follow_gimbal_yaw_control(vx_set, vy_set, wz_set, chassis_move_rc_to_vector);
    }
    else if ( chassis_move_rc_to_vector->chassis_mode == CHASSIS_VECTOR_NO_FOLLOW_YAW)
    {
        chassis_no_follow_control(vx_set, vy_set, wz_set, chassis_move_rc_to_vector);
    }
}




//��������״̬������
static void chassis_zero_force_control(float *vx_set, float *vy_set, float *wz_set, chassis_move_t *chassis_move_rc_to_vector)
{
    if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
    {
        return;
    }

    *vx_set = 0.0f;
    *vy_set = 0.0f;
    *wz_set = 0.0f;
}

//����ֹͣ״̬������
static void chassis_no_move_control(float *vx_set, float *vy_set, float *wz_set, chassis_move_t *chassis_move_rc_to_vector)
{
    if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
    {
        return;
    }
    *vx_set = 0.0f;
    *vy_set = 0.0f;
    *wz_set = 0.0f;
}

//����  ������̨״̬������
static void chassis_follow_gimbal_yaw_control(float *vx_set, float *vy_set, float *wz_set, chassis_move_t *chassis_move_rc_to_vector)
{
    if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
    {
        return;
    }
    chassis_rc_to_control_vector(vx_set, vy_set,chassis_move_rc_to_vector);
		
		*wz_set = -CHASSIS_PID_Calc(&chassis_move_rc_to_vector->chassis_follow_pid , chassis_move_rc_to_vector->error_aobut_gimbalYAW , 0 );
}

//���� �� ������̨״̬������
static void chassis_no_follow_control(float *vx_set, float *vy_set, float *wz_set, chassis_move_t *chassis_move_rc_to_vector)
{
    if (vx_set == NULL || vy_set == NULL || wz_set == NULL || chassis_move_rc_to_vector == NULL)
    {
        return;
    }

    chassis_rc_to_control_vector(vx_set, vy_set,chassis_move_rc_to_vector);

    if (chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_TURNLEFT_KEY)
    {
        *wz_set =  1.5;
    }
    else if(chassis_move_rc_to_vector->chassis_RC->key.v & CHASSIS_TURNRIGHT_KEY)
    {
        *wz_set = -1.5;
    }

}

