/**
 * Copyright (c) 2019, Freqchip
 * 
 * All rights reserved.
 * 
 * 
 */
#ifndef BLE_SIMPLE_CENTRAL_H
#define BLE_SIMPLE_CENTRAL_H

 /*
 * INCLUDES (����ͷ�ļ�)
 */
 #include "gap_api.h"
/*
 * MACROS (�궨��)
 */

/*
 * CONSTANTS (��������)
 */


/*
 * TYPEDEFS (���Ͷ���)
 */


/*
 * GLOBAL VARIABLES (ȫ�ֱ���)
 */


/*
 * LOCAL VARIABLES (���ر���)
 */
 
/*
 * LOCAL FUNCTIONS (���غ���)
 */

/*
 * EXTERN FUNCTIONS (�ⲿ����)
 */

/*
 * PUBLIC FUNCTIONS (ȫ�ֺ���)
 */

/*********************************************************************
 * @fn      app_gap_evt_cb
 *
 * @brief   Application layer GAP event callback function. Handles GAP evnets.
 *
 * @param   p_event - GAP events from BLE stack.
 *       
 *
 * @return  None.
 */
void app_gap_evt_cb(gap_event_t *event);

/*********************************************************************
 * @fn      simple_central_init
 *
 * @brief   Initialize simple central, BLE related parameters.
 *
 * @param   None. 
 *       
 *
 * @return  None.
 */
 void simple_central_init(void);

  
#endif
