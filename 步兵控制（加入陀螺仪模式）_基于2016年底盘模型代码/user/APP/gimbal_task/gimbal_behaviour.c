/**
  ****************************(C) COPYRIGHT 2019 IronSprit***********************
  * @file       gimbal_behaviour.c/h
  * @brief      �����̨������Ϊ�㡣
  * @note       ���ϵ�ʱ�Ȼ���pitch���ٻ���yaw,��ʱ�����ٶ���������ң�������ߣ�������
	              ʱ����̨�����ٶȾͻ������������ʼ��ʱ�䣬��û�����ͽ���ֹͣģʽ
								
								����취����ң�������ߣ�������ʱ��yaw��pitchͬʱ����
  * @history

  *  Version       Date            Author          status
  *  V2.0.0      2019-7-11         Young            ���
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 IronSprit************************
*/

#include "gimbal_behaviour.h"
#include "chassis_task.h"
#include "arm_math.h"
#include "Detect_Task.h"
#include "user_lib.h"

uint8_t  RemoteControl_Offline_Flag = 0;

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



/*---------------------------------------ģʽ�趨��Ϊ��------------------------------------*/


void gimbal_behaviour_mode_set(Gimbal_Control_t   *gimbal_mode_set)
{
    if (gimbal_mode_set == NULL)
    {
        return;
    }

    //��ʼ��ģʽ��
    if (gimbal_mode_set->gimbal_mode == GIMBAL_INIT)
    {
        static uint16_t init_time = 0;
        static uint16_t init_stop_time = 0;
			
        if (init_time < GIMBAL_INIT_TIME_MS)
        {
            init_time++;
        }
        //yaw��pitch�����relative_angle�Ǹ���Ŀ��λ�õ�ƫ��������ģ�������Ŀ��λ�ú�relative_angleΪ0
        if ( (fabsf(gimbal_mode_set->gimbal_yaw_motor.relative_angle - INIT_YAW_SET) < GIMBAL_INIT_ANGLE_ERROR) &&
                (fabsf(gimbal_mode_set->gimbal_pitch_motor.relative_angle - INIT_PITCH_SET) < GIMBAL_INIT_ANGLE_ERROR) )
        {
            //�����ʼ��λ��
            if (init_stop_time < GIMBAL_INIT_STOP_TIME_MS)
            {
                init_stop_time++;
            }
        }
        //������ʼ�����ʱ��6S�������Ѿ��ȶ���Ŀ��ֵһ��ʱ��200MS���˳���ʼ��״̬
        if ( (init_time < GIMBAL_INIT_TIME_MS) && (init_stop_time < GIMBAL_INIT_STOP_TIME_MS) )
        {
            return;
        }
        else
        {
            init_stop_time = 0;
            init_time = 0;
            gimbal_mode_set->gimbal_mode = GIMBAL_STOP;
					  RemoteControl_Offline_Flag = 0;
        }
    }
    //����ģʽ��
    else if( gimbal_mode_set->gimbal_mode == GIMBAL_RELAX )
    {
        if( toe_is_error(DBUSTOE)== 0 )
        {
            gimbal_mode_set->gimbal_mode = GIMBAL_INIT ;
					  RemoteControl_Offline_Flag = 1;
        }
    }
    //ֹͣģʽ��
    else if( gimbal_mode_set->gimbal_mode == GIMBAL_STOP )
    {
        if( toe_is_error(DBUSTOE)== 1 )                                            gimbal_mode_set->gimbal_mode = GIMBAL_RELAX;
        if( switch_is_down(gimbal_mode_set->gimbal_rc_ctrl->rc.s[MODEL_CHANNEL]))  gimbal_mode_set->gimbal_mode = GIMBAL_GYRO;
        if( switch_is_up(gimbal_mode_set->gimbal_rc_ctrl->rc.s[MODEL_CHANNEL])) 	 gimbal_mode_set->gimbal_mode = GIMBAL_ENCONDE;
    }
		 //������ģʽ��
    else if( gimbal_mode_set->gimbal_mode == GIMBAL_GYRO )
    {
        if( toe_is_error(DBUSTOE)== 1 )                                            gimbal_mode_set->gimbal_mode = GIMBAL_RELAX;
        if( switch_is_mid(gimbal_mode_set->gimbal_rc_ctrl->rc.s[MODEL_CHANNEL]))   gimbal_mode_set->gimbal_mode = GIMBAL_STOP;
        if( switch_is_up(gimbal_mode_set->gimbal_rc_ctrl->rc.s[MODEL_CHANNEL]))    gimbal_mode_set->gimbal_mode = GIMBAL_ENCONDE;
    }
    //��е��ģʽ��
    else if( gimbal_mode_set->gimbal_mode == GIMBAL_ENCONDE )
    {
        if( toe_is_error(DBUSTOE)== 1 )                                                   gimbal_mode_set->gimbal_mode = GIMBAL_RELAX;
        else if( switch_is_mid(gimbal_mode_set->gimbal_rc_ctrl->rc.s[MODEL_CHANNEL]))     gimbal_mode_set->gimbal_mode = GIMBAL_STOP;
        else if( switch_is_down(gimbal_mode_set->gimbal_rc_ctrl->rc.s[MODEL_CHANNEL]))    gimbal_mode_set->gimbal_mode = GIMBAL_GYRO;
        //else if( get_chassis_mode() == 1 )                                                 gimbal_mode_set->gimbal_mode = GIMBAL_KEY_TO_ALIGN;
    }
    //��̨��С���ܶ�������±��ֻ���
    else if( gimbal_mode_set->gimbal_mode == GIMBAL_KEY_TO_ALIGN )
    {
        if( toe_is_error(DBUSTOE)== 1 )                                                    gimbal_mode_set->gimbal_mode = GIMBAL_RELAX;
        else if( switch_is_mid(gimbal_mode_set->gimbal_rc_ctrl->rc.s[MODEL_CHANNEL]))      gimbal_mode_set->gimbal_mode = GIMBAL_STOP;
        else if( switch_is_down(gimbal_mode_set->gimbal_rc_ctrl->rc.s[MODEL_CHANNEL]))     gimbal_mode_set->gimbal_mode = GIMBAL_GYRO;
        //else if( get_chassis_mode() == 0 )                                                 gimbal_mode_set->gimbal_mode = GIMBAL_ENCONDE;
    }
}





/*-----------------------------------------����������Ϊ��-----------------------------------*/

/**
  * @brief          ��̨��Ϊ���ƣ����ݲ�ͬ��Ϊ���ò�ͬ���ƺ���
  * @author         RM
  * @param[in]      ���õ�yaw�Ƕ�����ֵ����λ rad
  * @param[in]      ���õ�pitch�Ƕ�����ֵ����λ rad
  * @param[in]      ��̨����ָ��
  * @retval         ���ؿ�
  */

void gimbal_behaviour_control_set(float *add_yaw, float *add_pitch, Gimbal_Control_t *gimbal_control_set)
{

    if (add_yaw == NULL || add_pitch == NULL || gimbal_control_set == NULL)
    {
        return;
    }

    static float rc_add_yaw = 0.0f, rc_add_pitch= 0.0f;
    static int16_t yaw_channel = 0, pitch_channel = 0;

    //ң���������ݴ�������
    rc_deadline_limit(gimbal_control_set->gimbal_rc_ctrl->rc.ch[YAW_CHANNEL], yaw_channel, GIMBAL_RC_DEADLINE);
    rc_deadline_limit(gimbal_control_set->gimbal_rc_ctrl->rc.ch[PITCH_CHANNEL], pitch_channel, GIMBAL_RC_DEADLINE);

    //ң����+�������ݵ���
    rc_add_yaw = yaw_channel * YAW_RC_SEN - gimbal_control_set->gimbal_rc_ctrl->mouse.x * YAW_MOUSE_SEN;
    rc_add_pitch = pitch_channel * PITCH_RC_SEN + gimbal_control_set->gimbal_rc_ctrl->mouse.y * PITCH_MOUSE_SEN;


    if (gimbal_control_set->gimbal_mode == GIMBAL_INIT)
    {
        gimbal_init_control(&rc_add_yaw, &rc_add_pitch, gimbal_control_set);
    }
    else if (gimbal_control_set->gimbal_mode == GIMBAL_RELAX)
    {
        gimbal_relax_control(&rc_add_yaw, &rc_add_pitch, gimbal_control_set);
    }
    else if (gimbal_control_set->gimbal_mode == GIMBAL_STOP )
    {
        gimbal_stop_control(&rc_add_yaw, &rc_add_pitch, gimbal_control_set);
    }
		else if (gimbal_control_set->gimbal_mode == GIMBAL_GYRO )
    {
        gimbal_gyro_control(&rc_add_yaw, &rc_add_pitch, gimbal_control_set);
    }
    else if (gimbal_control_set->gimbal_mode == GIMBAL_KEY_TO_ALIGN )
    {
        gimbal_key_to_align_control(&rc_add_yaw, &rc_add_pitch, gimbal_control_set);
    }
    else if (gimbal_control_set->gimbal_mode == GIMBAL_ENCONDE )
    {
        gimbal_encoder_control(&rc_add_yaw, &rc_add_pitch, gimbal_control_set);
    }

    //��������������ֵ
    *add_yaw = rc_add_yaw;
    *add_pitch = rc_add_pitch;
}


/**
  * @brief          ��ң��������ʱ����̨���ڷ���ģʽ����ʱ��̨����
  * @author         Young
  * @param[in]      yaw��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      pitch��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      ��̨����ָ��
  * @retval         ���ؿ�
  */
static void gimbal_relax_control(float *yaw, float *pitch, Gimbal_Control_t *gimbal_control_set)
{
    if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
    {
        return;
    }

    *yaw   = 0.0f;
    *pitch = 0.0f;
}

/**
  * @brief          ��̨��ʼ��ģʽ����
  * @author         Young
  * @param[in]      yaw��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      pitch��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      ��̨����ָ��
  * @retval         ���ؿ�
  * @waring         �ϵ�ʱ�Ȼ���pitch���ٻ���yaw�����ң�������ߣ��ٴ�����ʱ��pitch��yawͬʱ��������ʱ�����ٶȲ�������
  */
static void gimbal_init_control(float *yaw, float *pitch, Gimbal_Control_t *gimbal_control_set)
{
    if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
    {
        return;
    }
		
    if(RemoteControl_Offline_Flag)//��ң��������ʱ���ٴ����ߣ�yaw��pitchͬʱ��������ʱ��̨�����ٶ�����
		{
		    *pitch = (INIT_PITCH_SET - gimbal_control_set->gimbal_pitch_motor.relative_angle) * GIMBAL_INIT_PITCH_SPEED;
        *yaw = (INIT_YAW_SET - gimbal_control_set->gimbal_yaw_motor.relative_angle) * GIMBAL_INIT_YAW_SPEED;
		}
		else
		{
		    //��ʼ��״̬����������
				if ( fabsf(INIT_PITCH_SET - gimbal_control_set->gimbal_pitch_motor.relative_angle) > GIMBAL_INIT_ANGLE_ERROR )
				{
						*pitch = (INIT_PITCH_SET - gimbal_control_set->gimbal_pitch_motor.relative_angle) * GIMBAL_INIT_PITCH_SPEED;
						*yaw = 0.0f;
				}
				else
				{
						*pitch = (INIT_PITCH_SET - gimbal_control_set->gimbal_pitch_motor.relative_angle) * GIMBAL_INIT_PITCH_SPEED;
						*yaw = (INIT_YAW_SET - gimbal_control_set->gimbal_yaw_motor.relative_angle) * GIMBAL_INIT_YAW_SPEED;
				}
		}
}

/**
  * @brief          ��ֹ̨ͣģʽ���ƣ���ʱ��̨����
  * @author         Young
  * @param[in]      yaw��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      pitch��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      ��̨����ָ��
  * @retval         ���ؿ�
  */
static void gimbal_stop_control(float *yaw, float *pitch, Gimbal_Control_t *gimbal_control_set)
{
    if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
    {
        return;
    }
    *yaw =    0.0f;
    *pitch =  0.0f;
}

/**
  * @brief          ��̨�����ǿ��ƣ�����������ǽǶȿ��ƣ�
  * @author         Young
  * @param[in]      yaw��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      pitch��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      ��̨����ָ��
  * @retval         ���ؿ�
  */
#ifdef USED_GYRO
static void gimbal_gyro_control(float *yaw, float *pitch, Gimbal_Control_t *gimbal_control_set)
{
    if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
    {
        return;
    }
    //����Ҫ�����ɼ���һ����ͷ����
}
#endif


/**
  * @brief          ��̨һ���������
  * @author         Young
  * @param[in]      yaw��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      pitch��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ rad
  * @param[in]      ��̨����ָ��
  * @retval         ���ؿ�
  */
static void gimbal_key_to_align_control(float *yaw, float *pitch, Gimbal_Control_t *gimbal_control_set)
{
    if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
    {
        return;
    }
    
		*pitch = (INIT_PITCH_SET - gimbal_control_set->gimbal_pitch_motor.relative_angle) * GIMBAL_INIT_PITCH_SPEED;
		*yaw = (INIT_YAW_SET - gimbal_control_set->gimbal_yaw_motor.relative_angle) * GIMBAL_INIT_YAW_SPEED;
}


/**
  * @brief          ��̨����ֵ���ƣ��������ԽǶȿ��ƣ�
  * @author         Young
  * @param[in]      yaw��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ ��
  * @param[in]      pitch��Ƕȿ��ƣ�Ϊ�Ƕȵ����� ��λ ��
  * @param[in]      ��̨����ָ��
  * @retval         ���ؿ�
  */
static void gimbal_encoder_control(float *yaw, float *pitch, Gimbal_Control_t *gimbal_control_set)
{
    if (yaw == NULL || pitch == NULL || gimbal_control_set == NULL)
    {
        return;
    }
    //����Ҫ����
}
