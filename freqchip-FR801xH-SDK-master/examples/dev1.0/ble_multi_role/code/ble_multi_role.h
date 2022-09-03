/**
 * Copyright (c) 2019, Freqchip
 * 
 * All rights reserved.
 * 
 * 
 */
#ifndef BLE_MULTI_ROLE_H
#define BLE_MULTI_ROLE_H
 
 /*
 * INCLUDES (����ͷ�ļ�)
 */

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


/** @function group ble peripheral device APIs (ble������ص�API)
 * @{
 */
 
void app_gap_evt_cb(gap_event_t *p_event);


/*********************************************************************
 * @fn      multi_role_init
 *
 * @brief   Initialize multi role, including peripheral & central roles, 
 *          and BLE related parameters.
 *
 * @param   None. 
 *       
 *
 * @return  None.
 */
void multi_role_init(void);

#endif
