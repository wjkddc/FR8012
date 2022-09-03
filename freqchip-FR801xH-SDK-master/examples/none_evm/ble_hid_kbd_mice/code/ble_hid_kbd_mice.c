/**
 * Copyright (c) 2019, Tsingtao Freqchip
 *
 * All rights reserved.
 *
 *
 */

/*
* INCLUDES (����ͷ�ļ�)
*/
#include <stdbool.h>
#include "gap_api.h"
#include "gatt_api.h"
#include "hid_service.h"
#include "ble_hid_kbd_mice.h"
#include "gatt_sig_uuid.h"
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

/*********************************************************************
 * @fn      hid_start_adv
 *
 * @brief   Set advertising data, advertising response data
 *					and set adv configration parameters.
 *
 * @param   None
 *       	
 *
 * @return  None
 */
void hid_start_adv(void)
{
    // GAP - Advertisement data (max size = 31 bytes, though this is
    // best kept short to conserve power while advertisting)
    // GAP-�㲥��������,�31���ֽ�.��һ������ݿ��Խ�ʡ�㲥ʱ��ϵͳ����.
    uint8_t adv_data[0x1C] =
    {
        // appearance
        0x03,   // length of this data
        GAP_ADVTYPE_APPEARANCE,
        LO_UINT16(GAP_APPEARE_GENERIC_HID),
        HI_UINT16(GAP_APPEARE_GENERIC_HID),

        // service UUIDs, to notify central devices what services are included
        // in this peripheral. ����central������ʲô����, ��������ֻ��һ����Ҫ��.
        0x03,   // length of this data
        GAP_ADVTYPE_16BIT_COMPLETE,
        LO_UINT16(HID_SERV_UUID),
        HI_UINT16(HID_SERV_UUID),
    };
    *(uint16_t *)(adv_data+2) = gap_get_dev_appearance();

    // GAP - Scan response data (max size = 31 bytes, though this is
    // best kept short to conserve power while advertisting)
    // GAP-Scan response����,�31���ֽ�.��һ������ݿ��Խ�ʡ�㲥ʱ��ϵͳ����.
    uint8_t scan_rsp_data[0x1F] =
    {
        // complete name �豸����
        0x11,   // length of this data
        GAP_ADVTYPE_LOCAL_NAME_COMPLETE,
        'B',
        'L',
        'E',
        ' ',
        'H',
        'I',
        'D',
        ' ',
        'K',
        'B',
        'D',
        ' ',
        'M',
        'I',
        'C',
        'E',

        // Tx power level ���书��
        0x02,   // length of this data
        GAP_ADVTYPE_POWER_LEVEL,
        0,       // 0dBm
    };


    gap_adv_param_t adv_param;
    adv_param.adv_mode = GAP_ADV_MODE_UNDIRECT;
    adv_param.adv_addr_type = GAP_ADDR_TYPE_PUBLIC;
    adv_param.adv_chnl_map = GAP_ADV_CHAN_ALL;
    adv_param.adv_filt_policy = GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
    adv_param.adv_intv_min = 80;
    adv_param.adv_intv_max = 80;
    gap_set_advertising_param(&adv_param);

    gap_set_advertising_data(adv_data,sizeof(adv_data));
    gap_set_advertising_rsp_data(scan_rsp_data,sizeof(scan_rsp_data) );

    gap_start_advertising(0);
}




