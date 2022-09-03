/**
 * Copyright (c) 2019, Freqchip
 *
 * All rights reserved.
 *
 *
 */

/*
 * INCLUDES 
 */
#include <stdint.h>
#include <stdbool.h>

#include "sha256.h"
#include "co_printf.h"
#include "os_timer.h"
#include "os_mem.h"
#include "sys_utils.h"

#include "mesh_api.h"
#include "mesh_model_msg.h"
#include "ali_mesh_led_driver.h"
#include "ali_mesh_info.h"

#include "driver_plf.h"
#include "driver_system.h"
#include "driver_pmu.h"
#include "button.h"

#include "flash_usage_config.h"
#include "demo_clock.h"
#include "vendor_timer_ctrl.h"
/*
 * MACROS 
 */
//#define ALI_MESH_TIMER

/*
 * CONSTANTS 
 */
#define ALI_MESH_VERSION                1

#define ALI_MESH_50HZ_CHECK_IO          GPIO_PD6

/*
 * this is an ali mesh key sample: 0000009c,78da076b60cb,ee7751e0dad7483eb1c7391310b4a951
 * these information should be read from flash in actual product.
 */
uint8_t ali_mesh_key_bdaddr[] = {0xd9,0xbb,0x6b,0x07,0xda,0x78};
uint8_t ali_mesh_key_pid[] = {0x0e, 0x01, 0x00, 0x00};
uint8_t ali_mesh_key_secret[] = {0x92,0x37,0x41,0x48,0xb5,0x24,0xd9,0xcf,0x7c,0x24,0x04,0x36,0x0b,0xa8,0x91,0xd0};

/*
 * TYPEDEFS 
 */
typedef struct led_hsl_s
{
    uint16_t led_l;
    uint16_t led_h;
    uint16_t led_s;
}led_hsl_t;

/*
 * GLOBAL VARIABLES 
 */

/*
 * LOCAL VARIABLES 
 */

/*
 * LOCAL FUNCTIONS 
 */
static void app_mesh_recv_gen_onoff_led_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_gen_onoff_fan_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_lightness_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_hsl_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_CTL_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_vendor_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_stop_publish_msg_resend(void);

void app_mesh_start_publish_msg_resend(uint8_t * p_msg,uint8_t p_len);
void app_mesh_50Hz_check_timer_handler(void * arg);
void app_mesh_50Hz_check_enable(void);


// Mesh model define
static mesh_model_t light_models[] = 
{
    [0] = {
        .model_id = MESH_MODEL_ID_ONOFF,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_gen_onoff_led_msg,
    },
    [1] = {
        .model_id = MESH_MODEL_ID_LIGHTNESS,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_lightness_msg,
    },
    
    [2] = {
        .model_id = MESH_MODEL_ID_HSL,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_hsl_msg,
    },
    #if 1
    [3] = {
        .model_id = MESH_MODEL_ID_VENDOR_ALI,
        .model_vendor = true,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_vendor_msg,
    },
    #endif
};

static os_timer_t app_mesh_50Hz_check_timer;
static os_timer_t publish_msg_resend_t;
static uint8_t publish_msg_buff[32] = {0},publish_msg_len = 0,resend_count = 0;

#if ALI_MESH_VERSION == 1
/* binding index, from 0 to total_model_num-1 */
static uint8_t app_key_binding_count = 0;
/* the index of app key to be binded */
static uint16_t app_key_binding_id = 0;
/* subscription index, from 0 to total_model_num-1 */
static uint8_t subscription_count = 0;
//static uint8_t subscription_element = 0;
static uint8_t publish_count = 0;
struct ali_mesh_sub_map_t {
    uint16_t element_idx;
    uint16_t group_addr;
};
struct ali_mesh_pub_map_t {
    uint16_t element_idx;
    uint32_t model_id;
    uint16_t pub_addr;
};
static const struct ali_mesh_sub_map_t ali_mesh_sub_map[] =
{
    [0] = {
        .element_idx = 0,
        .group_addr = MESH_ALI_GROUP_ADDR_LED,
    },
    [1] = {
        .element_idx = 0,
        .group_addr = MESH_ALI_GROUP_ADDR_LED,
    },
    [2] = {
        .element_idx = 0,
        .group_addr = MESH_ALI_GROUP_ADDR_LED,
    },
    [3] = {
        .element_idx = 0,
        .group_addr = MESH_ALI_GROUP_ADDR_LED,
    },
};

static const struct ali_mesh_pub_map_t ali_mesh_pub_map[] = {
    [0] = {
        .element_idx = 0,
        .model_id = MESH_MODEL_ID_ONOFF,
        .pub_addr = MESH_ALI_PUBLISH_ADDR,
    },
    [1] = {
        .element_idx = 0,
        .model_id = MESH_MODEL_ID_LIGHTNESS,
        .pub_addr = MESH_ALI_PUBLISH_ADDR,
    },
    [2] = {
        .element_idx = 0,
        .model_id = MESH_MODEL_ID_HSL,
        .pub_addr = MESH_ALI_PUBLISH_ADDR,
    },
    [3] = {
        .element_idx = 0,
        .model_id = MESH_MODEL_ID_VENDOR_ALI,
        .pub_addr = MESH_ALI_PUBLISH_ADDR,
    },
};
#endif  // ALI_MESH_VERSION == 1

#if ALI_MESH_VERSION == 1
/*********************************************************************
 * @fn      app_mesh_find_group_addr
 *
 * @brief   find group address by element.
 *
 * @param   element - element index
 *
 * @return  None.
 */
static uint16_t app_mesh_find_group_addr(uint8_t element)
{
    for(uint8_t i=0; i<sizeof(ali_mesh_sub_map)/sizeof(ali_mesh_sub_map[0]); i++)
    {
        if(ali_mesh_sub_map[i].element_idx == element)
        {
            return ali_mesh_sub_map[i].group_addr;
        }
    }

    /* The program shouldn't be running here */
    return 0xc000;
}
#endif  // ALI_MESH_VERSION == 1

/*********************************************************************
 * @fn      app_mesh_status_send_rsp
 *
 * @brief   used to send response to remote after receiving acknowledged-message.
 *
 * @param   ind     - message received from remote node
 *          opcode  - opcode field should be set in response message
 *          msg     - response message pointer
 *          msg_len - response message length
 *
 * @return  None.
 */
static void app_mesh_status_send_rsp(mesh_model_msg_ind_t const *ind, uint32_t opcode, uint8_t *msg, uint16_t msg_len)
{
    mesh_rsp_msg_t * p_rsp_msg = (mesh_rsp_msg_t *)os_malloc((sizeof(mesh_rsp_msg_t)+msg_len));
    p_rsp_msg->element_idx = ind->element;
    p_rsp_msg->app_key_lid = ind->app_key_lid;
    p_rsp_msg->model_id = ind->model_id;
    p_rsp_msg->opcode = opcode;
    p_rsp_msg->dst_addr = ind->src;
    p_rsp_msg->msg_len = msg_len;
    memcpy(p_rsp_msg->msg, msg, msg_len);

    mesh_send_rsp(p_rsp_msg);
    os_free(p_rsp_msg);
}

/*********************************************************************
 * @fn      app_mesh_send_dev_rst_ind
 *
 * @brief   The response opration when user delete the network.
 *
 * @param   None
 *
 * @return  None.
 */
static void app_mesh_send_dev_rst_ind(void)
{
    uint16_t option = 0;

    mesh_publish_msg_t *msg = (mesh_publish_msg_t *)os_malloc(sizeof(mesh_publish_msg_t) + 4);
    msg->element_idx = 0;
    msg->model_id = MESH_MODEL_ID_VENDOR_ALI;
    msg->opcode = MESH_VENDOR_INDICATION;
    
    msg->msg_len = 4;
    msg->msg[0] = 0x01;
    option = MESH_EVENT_UPDATA_ID;
    memcpy(&(msg->msg[1]), (uint8_t *)&option, 2);
    msg->msg[3] = MESH_EVENT_DEV_RST;

    mesh_publish_msg(msg);
    app_mesh_start_publish_msg_resend(msg->msg,msg->msg_len);
    os_free(msg);
}

/*********************************************************************
 * @fn      app_auto_update_led_state
 *
 * @brief   Actively report the state of the led when user change the state.
 *
 * @param   state     - on/off state
 *
 * @return  None.
 */
void app_auto_update_led_state(uint8_t state)
{
    uint8_t on_off_state[] = {0x00, 0x00, 0x01, 0x00};
    static uint8_t on_off_tid = 0;
    mesh_publish_msg_t *msg = (mesh_publish_msg_t *)os_malloc(sizeof(mesh_publish_msg_t) + sizeof(on_off_state));
    msg->element_idx = 0;
    msg->model_id = MESH_MODEL_ID_VENDOR_ALI;
    msg->opcode = MESH_VENDOR_INDICATION;

    on_off_state[0] = on_off_tid++;
    on_off_state[3] = state;
    
    memcpy(msg->msg, on_off_state, sizeof(on_off_state));
    msg->msg_len = sizeof(on_off_state);
    
    mesh_publish_msg(msg);
    app_mesh_start_publish_msg_resend(msg->msg,msg->msg_len);
    os_free((void *)msg);
}

/*********************************************************************
 * @fn      app_mesh_publish_msg_resend
 *
 * @brief   Resend publish msg if TiMao not reply.
 *
 * @param   arg     - param of timer callback.
 *
 * @return  None.
 */
static void app_mesh_publish_msg_resend(void * arg)
{
    if(!publish_msg_len)
        return;
        
    mesh_publish_msg_t *msg = (mesh_publish_msg_t *)os_malloc(sizeof(mesh_publish_msg_t) + publish_msg_len);
    msg->element_idx = 0;
    msg->model_id = MESH_MODEL_ID_VENDOR_ALI;
    msg->opcode = MESH_VENDOR_INDICATION;

    memcpy(msg->msg, publish_msg_buff, publish_msg_len);
    msg->msg_len = publish_msg_len;
    
    mesh_publish_msg(msg);
    os_free((void *)msg);

    //co_printf("=publish_msg_resend=\r\n");
    resend_count++;
    if(resend_count > 3)
        app_mesh_stop_publish_msg_resend();
}

/*********************************************************************
 * @fn      app_mesh_start_publish_msg_resend
 *
 * @brief   Start loop timer to resend publish msg.
 *
 * @param   p_msg     - publish msg buff.
 *
 * @param   p_len     - publish msg length.
 *
 * @return  None.
 */
void app_mesh_start_publish_msg_resend(uint8_t * p_msg,uint8_t p_len)
{
    if(p_len < 32)
    {
        resend_count = 0;
        memcpy(publish_msg_buff,p_msg,p_len);
        publish_msg_len = p_len;
    
        os_timer_start(&publish_msg_resend_t,2000,true);
    }
}

/*********************************************************************
 * @fn      app_mesh_stop_publish_msg_resend
 *
 * @brief   Stop the timer that resend publish msg.
 *
 * @param   None
 *
 * @return  None.
 */
static void app_mesh_stop_publish_msg_resend(void)
{
    //co_printf("=stop_publish_msg_resend=\r\n");
    os_timer_stop(&publish_msg_resend_t);
    memset(publish_msg_buff,0,32);
    publish_msg_len = 0;
    resend_count = 0;
}

/*********************************************************************
 * @fn      app_heartbeat_send
 *
 * @brief   A example for sending heartbeat packets.
 *
 * @param   None
 *
 * @return  None.
 */
void app_heartbeat_send(void)
{
    uint8_t heartbeat_data[] = {0x00,0x00,0x01,0x00,0x0E,0x01,0x10,0x27,0x0F,0x01,0x35,0x00,0x64,0x01,0x03,0x00,0x14,   \
                                    0x04,0x00,0x06,0x05,0x00,0x0D,0x05,0x01,0x04,0xF0,0x27,0x00};
    static uint8_t heartbeat_id = 0;
    mesh_publish_msg_t *msg = (mesh_publish_msg_t *)os_malloc(sizeof(mesh_publish_msg_t) + sizeof(heartbeat_data));
    msg->element_idx = 0;
    msg->model_id = MESH_MODEL_ID_VENDOR_ALI;
    msg->opcode = MESH_VENDOR_STATUS;
    heartbeat_id++;
    heartbeat_data[0] = heartbeat_id;
    memcpy(msg->msg, heartbeat_data, sizeof(heartbeat_data));
    msg->msg_len = sizeof(heartbeat_data);
    
    mesh_publish_msg(msg);
    //app_mesh_start_publish_msg_resend(msg->msg,msg->msg_len);
    os_free((void *)msg);
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to generic on-off
 *          model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_gen_onoff_led_msg(mesh_model_msg_ind_t const *ind)
{
    struct mesh_gen_onoff_model_status_t status;
    struct mesh_gen_onoff_model_set_t *onoff_set;
    
    if((ind->opcode != MESH_GEN_ONOFF_SET) && (ind->opcode != MESH_GEN_ONOFF_SET_UNACK)){
        return;
    }

    onoff_set = (struct mesh_gen_onoff_model_set_t *)ind->msg;
    app_led_set_onoffstate(onoff_set->onoff);
    //app_led_set_onoffstate(1, onoff_set->onoff);
    //co_printf("light on off led:%x,%x\r\n",ind->model_id,onoff_set->onoff);

    if(ind->opcode == MESH_GEN_ONOFF_SET) {
        status.present_onoff = app_led_get_onoffstate(0);
        status.target_onoff = app_led_get_onoffstate(0);
        status.remain = 0;
        app_mesh_status_send_rsp(ind, MESH_GEN_ONOFF_STATUS, (uint8_t *)&status, sizeof(status));
    }
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to generic on-off
 *          model of fan element or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_gen_onoff_fan_msg(mesh_model_msg_ind_t const *ind)
{
    struct mesh_gen_onoff_model_status_t status;
    struct mesh_gen_onoff_model_set_t *onoff_set;
    
    if((ind->opcode != MESH_GEN_ONOFF_SET) && (ind->opcode != MESH_GEN_ONOFF_SET_UNACK)){
        return;
    }

    onoff_set = (struct mesh_gen_onoff_model_set_t *)ind->msg;
    app_led_set_onoffstate(onoff_set->onoff);
    //app_led_set_onoffstate(1, onoff_set->onoff);
    //co_printf("light on off fan:%x,%x\r\n",ind->model_id,onoff_set->onoff);

    if(ind->opcode == MESH_GEN_ONOFF_SET) {
        status.present_onoff = app_led_get_onoffstate(0);
        status.target_onoff = app_led_get_onoffstate(0);
        status.remain = 0;
        app_mesh_status_send_rsp(ind, MESH_GEN_ONOFF_STATUS, (uint8_t *)&status, sizeof(status));
    }
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to lightness
 *          model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_lightness_msg(mesh_model_msg_ind_t const *ind)
{
    struct mesh_lightness_model_status_t status;
    struct mesh_lightness_model_set_t *lightness_set;
    
    lightness_set = (struct mesh_lightness_model_set_t *)ind->msg;
    if((ind->opcode != MESH_LIGHTNESS_SET) && (ind->opcode != MESH_LIGHTNESS_SET_UNACK)){
        return;
    }

    app_led_set_lightness(lightness_set->level);
    //app_led_set_level(1, lightness_set->level);
    //co_printf("light lightness:%x\r\n",lightness_set->level);
    if(ind->opcode == MESH_LIGHTNESS_SET)
    {
        status.current_level = app_led_get_ctl_lightness();
        status.target_level = lightness_set->level;
        status.remain = 0;
        app_mesh_status_send_rsp(ind, MESH_LIGHTNESS_STATUS, (uint8_t *)&status,sizeof(status));
    }
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to hsl
 *          model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_hsl_msg(mesh_model_msg_ind_t const *ind)
{
    struct mesh_hsl_model_status_t status;
    struct mesh_hsl_model_set_t *hsl_set;
    
    if((ind->opcode != MESH_HSL_SET) && (ind->opcode != MESH_HSL_SET_UNACK)){
        return;
    }
    //co_printf("hsl opcode:%x\r\n",ind->opcode);
    hsl_set = (struct mesh_hsl_model_set_t *)ind->msg;

    app_led_set_hsl(hsl_set->hue,hsl_set->hsl_saturation,hsl_set->lightness);
    //app_led_set_hsl(1,hsl_set->hue,hsl_set->hsl_saturation,hsl_set->lightness);
    if(ind->opcode == MESH_HSL_SET)
    {
        status.hsl_lightness = hsl_set->lightness;
        status.hsl_hue = hsl_set->hue;
        status.hsl_saturation = hsl_set->hsl_saturation;
        status.remain = 0;
        app_mesh_status_send_rsp(ind, MESH_HSL_STATUS, (uint8_t *)&status, sizeof(status));
    }
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to CTL
 *          model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_CTL_msg(mesh_model_msg_ind_t const *ind)
{
    struct mesh_CTL_model_status_t status;
    struct mesh_CTL_model_set_t *ctl_set;
    
    if((ind->opcode != MESH_TEMPERATURE_SET) && (ind->opcode != MESH_TEMPERATURE_SET_UNACK)){
        return;
    }
    
    ctl_set = (struct mesh_CTL_model_set_t *)ind->msg;
    app_led_set_CTL(ctl_set->lightness, ctl_set->temperature);
    //app_led_set_CTL(1, ctl_set->lightness, ctl_set->temperature);
    //co_printf("temperature:%x\r\n",ctl_set->temperature);
    
    if(ind->opcode == MESH_TEMPERATURE_SET)
    {
        status.current_lightness = app_led_get_ctl_lightness();
        status.current_temperature = app_led_get_ctl_temperature();
        status.target_lightness = app_led_get_ctl_lightness();
        status.target_temperature = app_led_get_ctl_temperature();
        status.remain = 0;
        app_mesh_status_send_rsp(ind, MESH_TEMPERATURE_STATUS, (uint8_t *)&status, sizeof(status));
    }
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to vendor
 *          defined model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_vendor_msg(mesh_model_msg_ind_t const *ind)
{
    struct mesh_vendor_model_set_new_t *vendor_set;
    led_hsl_t * led_hsl_p;
    
    if((ind->opcode != MESH_VENDOR_SET) 
        && (ind->opcode != MESH_VENDOR_SET_UNACK)
        && (ind->opcode != MESH_VENDOR_CONFIRMATION)){
        return;
    }
        
    //co_printf("vendor opcode:%x\r\n",ind->opcode);
    vendor_set = (struct mesh_vendor_model_set_new_t *)ind->msg;
    //vendor_set->attr_parameter = (uint8_t *)&ind->msg[3];

    //vendor_set_msg_handler(ind);
#ifdef ALI_MESH_TIMER
    vendor_set_timer_case(ind);
#endif    
    switch(vendor_set->attr_type)
    {
        case 0x121: // lightness
            break;
        case 0x122: // temperature
            break;    
        case 0x0123: // hsl
            led_hsl_p = (led_hsl_t * )&(vendor_set->attr_parameter);
            co_printf("h=%x,s=%x,l=%x\r\n",led_hsl_p->led_h,led_hsl_p->led_s,led_hsl_p->led_l);
            app_led_set_hsl(led_hsl_p->led_h,led_hsl_p->led_s,led_hsl_p->led_l);
            break;
        
    }
    //app_led_set_hsl(1, hsl_set->hue);
    if(ind->opcode == MESH_VENDOR_SET)
    {
        app_mesh_status_send_rsp(ind, MESH_VENDOR_STATUS, (uint8_t *)vendor_set, ind->msg_len);
    }
    else if(ind->opcode == MESH_VENDOR_CONFIRMATION)
    {
        app_mesh_stop_publish_msg_resend();
    }
}

/*********************************************************************
 * @fn      mesh_callback_func
 *
 * @brief   mesh message deal callback function.
 *
 * @param   event   - event to be processed
 *
 * @return  None.
 */
static void mesh_callback_func(mesh_event_t * event)
{
    uint8_t tmp_data[16];

    //co_printf("mesh_callback_func: %d.\r\n", event->type);
    switch(event->type) {
        case MESH_EVT_READY:
            mesh_start();
            break;
        case MESH_EVT_STARTED:
            app_led_init();
            //app_mesh_50Hz_check_enable();
            break;
        case MESH_EVT_STOPPED:
            system_sleep_enable();
            break;
        case MESH_EVT_IN_NETWORK:
            co_printf("device already in network\r\n");
            break;    
        case MESH_EVT_RESET:
            co_printf("removed from network by provisoner.\r\n");
            mesh_info_clear();
			app_mesh_user_data_clear();
            platform_reset_patch(0);
            break;
        case MESH_EVT_PROV_PARAM_REQ:
            tmp_data[0] = 0xa8;
            tmp_data[1] = 0x01;
            tmp_data[2] = 0x71;
            memcpy(&tmp_data[3], ali_mesh_key_pid, 4);
            memcpy(&tmp_data[7], ali_mesh_key_bdaddr, 6);
#if ALI_MESH_VERSION == 1
            tmp_data[13] = 0x02;
#else   // ALI_MESH_VERSION == 1
            tmp_data[13] = 0x00;
#endif  // ALI_MESH_VERSION == 1
            tmp_data[14] = 0x00;
            tmp_data[15] = 0x00;
            mesh_send_prov_param_rsp((uint8_t *)tmp_data, 0xd97478b3, 0, 0, 0, 0, 0, 0, 0, 1, 0);
            break;
        case MESH_EVT_PROV_AUTH_DATA_REQ:
            sha256_gen_auth_value((BYTE *)ali_mesh_key_pid, (BYTE *)ali_mesh_key_bdaddr, (BYTE *)ali_mesh_key_secret, tmp_data);
            mesh_send_prov_auth_data_rsp(true, 16, (uint8_t *)tmp_data);
            break;
        case MESH_EVT_PROV_RESULT:
            co_printf("result=%d\r\n",event->param.prov_result.state);
            break;
        case MESH_EVT_UPDATE_IND:
#if ALI_MESH_VERSION == 1
            if(event->param.update_ind->upd_type == MESH_UPD_TYPE_APP_KEY_UPDATED) // message type is app key update
            {
                mesh_appkey_t *appkey = (mesh_appkey_t *)event->param.update_ind->data;
                app_key_binding_id = appkey->appkey_id;
                mesh_model_bind_appkey(light_models[app_key_binding_count].model_id,
                                      light_models[app_key_binding_count].element_idx,
                                      app_key_binding_id);
                app_key_binding_count++;
            }
#else   // ALI_MESH_VERSION == 1
            #if 0
            if(event->param.update_ind->upd_type == MESH_UPD_TYPE_SUBS_LIST) // message type is app key update
            {
                mesh_subs_t *sub = (mesh_subs_t *)event->param.update_ind->data;
                uint16_t group_addr;
                uint16_t element;
                
                element = sub->element_addr;
                group_addr = sub->list[1] | (sub->list[0] << 8);
                for(uint8_t i=0; i<sizeof(light_models)/sizeof(light_models[0]); i++)
                {
                    if((light_models[i].element_idx == element)
                        && (light_models[i].model_id != sub->model_id))
                    {
                        mesh_model_sub_group_addr(light_models[i].model_id,
                                                    element,
                                                    group_addr);
                    }
                }
            }
            #endif
#endif  // ALI_MESH_VERSION == 1
            app_mesh_store_info_timer_start(2000);
            break;
#if ALI_MESH_VERSION == 1
        case MESH_EVT_MODEL_APPKEY_BINDED:
			if(app_key_binding_count < sizeof(light_models)/sizeof(light_models[0]))
			{
				mesh_model_bind_appkey(light_models[app_key_binding_count].model_id,
                                        light_models[app_key_binding_count].element_idx,
                                        app_key_binding_id);
				app_key_binding_count++;
			}
			else
			{
                app_key_binding_count = 0;
				mesh_model_sub_group_addr(light_models[subscription_count].model_id,
                                            light_models[subscription_count].element_idx,
                                            app_mesh_find_group_addr(light_models[subscription_count].element_idx));
                /* only subscript the primary model inside one element, similar as ALI MESH VERSION 0 */
                uint8_t element_idx = light_models[subscription_count].element_idx;
                //for(; ((subscription_count<sizeof(light_models)/sizeof(light_models[0])) && (light_models[subscription_count].element_idx == element_idx));)
                if(((subscription_count<sizeof(light_models)/sizeof(light_models[0])) && (light_models[subscription_count].element_idx == element_idx)))
                {
                    subscription_count++;
                }
			}
			break;
		case MESH_EVT_MODEL_GRPADDR_SUBED:
			if(subscription_count < sizeof(light_models)/sizeof(light_models[0]))
			{
				mesh_model_sub_group_addr(light_models[subscription_count].model_id,
                                            light_models[subscription_count].element_idx,
                                            app_mesh_find_group_addr(light_models[subscription_count].element_idx));
                /* only subscript the primary model inside one element, similar as ALI MESH VERSION 0 */
                uint8_t element_idx = light_models[subscription_count].element_idx;
				//for(; ((subscription_count<sizeof(light_models)/sizeof(light_models[0])) && (light_models[subscription_count].element_idx == element_idx));)
                if(((subscription_count<sizeof(light_models)/sizeof(light_models[0])) && (light_models[subscription_count].element_idx == element_idx)))
                {
                    subscription_count++;
                }
            }
            else
            {
                subscription_count = 0;
                uint16_t src_id = 0;
                uint8_t appkey = 0;
                mesh_get_remote_param(&src_id,&appkey);
                
                mesh_model_add_publish_addr(ali_mesh_pub_map[publish_count].model_id,
                                            ali_mesh_pub_map[publish_count].element_idx,
                                            ali_mesh_pub_map[publish_count].pub_addr,
                                            appkey,
                                            7,
                                            0,
                                            0,
                                            0,
                                            NULL);
                publish_count++;
            }
            break;
        case MESH_EVT_MODEL_ADDR_PUBLISHED:
            if(publish_count < sizeof(ali_mesh_pub_map)/sizeof(ali_mesh_pub_map[0])) {
                uint16_t src_id = 0;
                uint8_t appkey = 0;
                mesh_get_remote_param(&src_id,&appkey);
                
                mesh_model_add_publish_addr(ali_mesh_pub_map[publish_count].model_id,
                                            ali_mesh_pub_map[publish_count].element_idx,
                                            ali_mesh_pub_map[publish_count].pub_addr,
                                            appkey,
                                            7,
                                            0,
                                            0,
                                            0,
                                            NULL);
                publish_count++;
            }
            else {
                publish_count = 0;
                uint16_t src_id = 0;
                uint8_t appkey = 0;
                mesh_get_remote_param(&src_id,&appkey);
                app_led_set_remote_msg(src_id,appkey);
            }
			break;
#endif  // ALI_MESH_VERSION == 1
        case MESH_EVT_RECV_MSG:
            {
                const mesh_model_msg_ind_t *ind = &(event->param.model_msg);
                co_printf("model_id: 0x%04x\r\n", ind->model_id);
                co_printf("opcode: 0x%08x\r\n", ind->opcode);
                co_printf("src: 0x%04x\r\n", ind->src);
                co_printf("msg: ");
                for(uint8_t i=0; i<ind->msg_len; i++)
                {
                    co_printf("%02x ", ind->msg[i]);
                }
                co_printf("\r\n");
    			for(uint8_t i=0; i < sizeof(light_models)/sizeof(light_models[0]); i++)
    			{
    				//if((event->param.model_msg.model_id == light_models[i].model_id)
                    //    && (event->param.model_msg.element == light_models[i].element_idx))
                    /* 
                     * lower layer of mesh will loop all models for group address (AKA message destination
                     * address) match checking, and only the primary model inside one element subscript
                     * these group address. The result is that the model_id field inside this message
                     * will only match the primary model, even the opcode is used by the other models.
                     * So only element field is checked here, and the code inside msg_handler takes
                     * responsibility to check which model has to deal with this mesasge.
                     */
                    if((ind->element == light_models[i].element_idx) && (ind->model_id == light_models[i].model_id))
    				{
    					light_models[i].msg_handler(ind);
    				}
    			}
            }
			break;
        case MESH_EVT_ADV_REPORT:
            {
                #if 0
                gap_evt_adv_report_t *report = &(event->param.adv_report);
                co_printf("recv adv from: %02x-%02x-%02x-%02x-%02x-%02x\r\n", report->src_addr.addr.addr[5],
                                                                                report->src_addr.addr.addr[4],
                                                                                report->src_addr.addr.addr[3],
                                                                                report->src_addr.addr.addr[2],
                                                                                report->src_addr.addr.addr[1],
                                                                                report->src_addr.addr.addr[0]);
                for(uint16_t i=0; i<report->length; i++) {
                    co_printf("%02x ", report->data[i]);
                }
                co_printf("\r\n");
                #endif
            }
            break;
        default:
            break;
    }
}

/*********************************************************************
 * @fn      app_mesh_led_init
 *
 * @brief   init mesh model, set callback function, set feature supported by
 *          this application, add models, etc.
 *
 * @param   None.
 *
 * @return  None.
 */
void app_mesh_led_init(void)
{
    if(app_mesh_ali_info_check_valid())
    {
        app_mesh_ali_info_load_key(ali_mesh_key_secret);
        app_mesh_ali_info_load_bdaddr(ali_mesh_key_bdaddr);
        app_mesh_ali_info_load_pid(ali_mesh_key_pid);
    }
    mesh_set_cb_func(mesh_callback_func);
    
    mesh_init(MESH_FEATURE_RELAY, MESH_INFO_STORE_ADDR);

    for(uint8_t i=0; i<sizeof(light_models)/sizeof(light_models[0]); i++)
    {
        mesh_add_model(&light_models[i]);
    }
    
    app_mesh_store_info_timer_init();
    os_timer_init(&app_mesh_50Hz_check_timer, app_mesh_50Hz_check_timer_handler, NULL);
    os_timer_init(&publish_msg_resend_t,app_mesh_publish_msg_resend,NULL);
#ifdef ALI_MESH_TIMER
    sys_timer_init();
#endif
#if ALI_MESH_VERSION == 1
    app_key_binding_count = 0;
    app_key_binding_id = 0;
    subscription_count = 0;
    publish_count = 0;
#endif  // ALI_MESH_VERSION == 1
}

/*********************************************************************
 * @fn      app_mesh_50Hz_check_enable
 *
 * @brief   enable 50Hz check, enable extern interrupt, start timer.
 *
 * @param   arg     - useless
 *
 * @return  None.
 */
void app_mesh_50Hz_check_enable(void)
{
    /* use PA0 for 50Hz detection */
    pmu_port_wakeup_func_set(ALI_MESH_50HZ_CHECK_IO);
    NVIC_EnableIRQ(PMU_IRQn);

    /* start timer */
    os_timer_start(&app_mesh_50Hz_check_timer, 100, false);
}

/*********************************************************************
 * @fn      ali_mesh_50Hz_check_timer_handler
 *
 * @brief   trigger of this timer means AC power is removed.
 *
 * @param   arg     - useless
 *
 * @return  None.
 */
void app_mesh_50Hz_check_timer_handler(void * arg)
{
    mesh_stop();
}

/*********************************************************************
 * @fn      pmu_gpio_isr_ram
 *
 * @brief   interrupt handler of 50Hz voltage level changing.
 *
 * @param   arg     - useless
 *
 * @return  None.
 */
__attribute__((section("ram_code"))) void pmu_gpio_isr_ram(void)
{
    static uint32_t last_value = 0;
    uint32_t curr_value;
    
    /* get current gpio value and initial gpio last value */
    curr_value = ool_read32(PMU_REG_GPIOA_V);
    ool_write32(PMU_REG_PORTA_LAST, curr_value);

    #if 0
    button_toggle_detected(curr_value);
    #else
    if((curr_value ^ last_value) & ALI_MESH_50HZ_CHECK_IO) {
        /* restart timer */
        os_timer_start(&app_mesh_50Hz_check_timer, 100, false);
    }
    #endif
    last_value = curr_value;
}


/*********************************************************************
 * @fn      ali_mesh_send_user_adv_packet
 *
 * @brief   this is an example to show how to send data defined by user, 
 *          this function should not be recall until event GAP_EVT_ADV_END 
 *          is received.
 *
 * @param   duration    - how many 10ms advertisng will last.
 *          adv_data    - advertising data pointer
 *          adv_len     - advertising data length
 *
 * @return  None.
 */
void app_mesh_send_user_adv_packet(uint8_t duration, uint8_t *adv_data, uint8_t adv_len)
{
    gap_adv_param_t adv_param;

    adv_param.adv_mode = GAP_ADV_MODE_NON_CONN_NON_SCAN;
    adv_param.adv_addr_type = GAP_ADDR_TYPE_PRIVATE;
    adv_param.adv_intv_min = 32;
    adv_param.adv_intv_max = 32;
    adv_param.adv_chnl_map = GAP_ADV_CHAN_ALL;
    adv_param.adv_filt_policy = GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
    gap_set_advertising_param(&adv_param);
    gap_set_advertising_data(adv_data, adv_len);
    gap_start_advertising(duration);
}

