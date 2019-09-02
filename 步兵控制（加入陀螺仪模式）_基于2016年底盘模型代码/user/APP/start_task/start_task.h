#ifndef _start_task_H
#define _start_task_H

#include "FreeRTOS.h"
#include "task.h"

/*--------������ʼ����-------*/
#define START_TASK_PRIO 1            //�������ȼ�
#define START_STK_SIZE  512          //�����ջ��С

/*--------�����û�����-------*/
#define User_TASK_PRIO  2
#define User_STK_SIZE   120

/*--------�����Լ�����-------*/
#define Detect_TASK_PRIO  3
#define Detect_STK_SIZE   512

/*--------������������-------*/
#define Chassis_TASK_PRIO   4
#define Chassis_STK_SIZE    512

/*--------������̨����-------*/
#define Gimbal_TASK_PRIO    5
#define Gimbal_STK_SIZE     512

void Create_MyTask(void);
void start_task(void *pvParameters);

#endif

