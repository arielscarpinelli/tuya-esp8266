#define __DEVICE_GLOBALS
#include "tuya_smart_api.h"
#include "wf_sdk_adpt.h"

#define PRODUCT_KEY "cgdqy92fzdjhi62a"

#define DEVICE_MOD "device_mod"
#define DEVICE_PART "device_part"

#define D0 GPIO_ID_PIN(16)
#define D1 GPIO_ID_PIN(5)
#define D2 GPIO_ID_PIN(4)
#define D3 GPIO_ID_PIN(0)
#define D4 GPIO_ID_PIN(2)

#define LED_BUILTIN D4
#define LED_SECONDARY D0
#define RELAY D1
#define BUTTON_PIN D2
#define BUTTON_PIN_PERIPHS PERIPHS_IO_MUX_GPIO4_U
#define DPID 1
#define DPIDS "1"

#define PRESS_HOLD_TIME 3000

STATIC VOID key_process(INT gpio_no, PUSH_KEY_TYPE_E type, INT cnt);

TIMER_ID wfl_timer;
STATIC VOID wfl_timer_cb(UINT timerID, PVOID pTimerArg);
VOID device_cb(SMART_CMD_E cmd, cJSON *root);
STATIC VOID all_channel_stat_upload(VOID);

STATIC bool state = false;
STATIC LED_HANDLE wifi_stat_led_handle;
STATIC LED_HANDLE state_led_handle;
STATIC LED_HANDLE relay_handle;

// call back function
VOID set_firmware_tp(IN OUT CHAR *firm_name, IN OUT CHAR *firm_ver) {
	strcpy(firm_name, APP_BIN_NAME);
	strcpy(firm_ver, USER_SW_VER);
	return;
}

/**
 * @brief gpio 测试
 * 
 * @return BOOL 
 */
BOOL gpio_func_test(VOID) {
	return TRUE;
}

/**
 * @brief pre_app_init 初始化
 *        比app_init 提前调用
 * 
 * @return VOID 
 */
VOID pre_app_init(VOID) {
    set_console(FALSE);
    PR_NOTICE("pre_app_init");

	uint8 gpio_out_config[] = {RELAY, LED_BUILTIN, LED_SECONDARY};
	tuya_pre_app_set_gpio_out(gpio_out_config, CNTSOF(gpio_out_config));
	return;
}

/**
 * @brief 产测接口函数
 * 
 * @param flag 产测标志
 * @param rssi 当前产测路由 RSSI值
 * @return VOID 
 */
VOID prod_test(BOOL flag, CHAR rssi) {
    // TODO: 信号值测试
    if (flag == FALSE) {
        PR_ERR("no auth");
        return;
    }

    PR_NOTICE("product test mode");
    OPERATE_RET op_ret;  // 注册操作的返回值

}



/**
 * @brief app_init 初始化
 * 
 * @return VOID 
 */
VOID app_init(VOID) {

	/* 设置WIFI 模式 */
    tuya_app_cfg_set(WCM_LOW_POWER, prod_test);
}

/**
 * @brief 查询回调函数，推送设备当前状态
 * 
 */
STATIC VOID dp_qeury_cb(IN CONST TY_DP_QUERY_S *dp_qry) {
	all_channel_stat_upload();
}

/**
 * @brief device_init 设备初始化
 * 
 * @return OPERATE_RET 
 */
OPERATE_RET device_init(VOID) {
	PR_NOTICE("fireware info name:%s version:%s", APP_BIN_NAME, USER_SW_VER);
	OPERATE_RET op_ret;

	op_ret = tuya_device_init(PRODUCT_KEY, device_cb, USER_SW_VER);
	if (op_ret != OPRT_OK) {
		return op_ret;
	}

	TY_IOT_CBS_S wf_cbs = {
		.dev_dp_query_cb = dp_qeury_cb,
		.ug_reset_inform_cb = NULL,
	};
	gw_register_cbs(&wf_cbs);

	op_ret = tuya_psm_register_module(DEVICE_MOD, DEVICE_PART);
	if (op_ret != OPRT_OK && op_ret != OPRT_PSM_E_EXIST) {
		PR_ERR("tuya_psm_register_module error:%d", op_ret);
		return op_ret;
	}

	op_ret = tuya_create_led_handle(LED_BUILTIN, &wifi_stat_led_handle);
	if (op_ret != OPRT_OK) {
		return op_ret;
	}
	tuya_set_led_type(wifi_stat_led_handle, OL_HIGH, 0);

	op_ret = tuya_create_led_handle(LED_SECONDARY, &state_led_handle);
	if (op_ret != OPRT_OK) {
		return op_ret;
	}
	tuya_set_led_type(state_led_handle, OL_HIGH, 0);

	op_ret = tuya_create_led_handle(RELAY, &relay_handle);
	if (op_ret != OPRT_OK) {
		return op_ret;
	}
	tuya_set_led_type(relay_handle, OL_LOW, 0);
	/*
GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(pnum)),
                   GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(pnum))) |
                   GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE));      //enable open drain;
    */
    
    // We are using D3 as a gnd source for the push button. It is useful in D1 mini boards where
    // there is only one gnd output that you may use to feed the relay.
    LED_HANDLE gnd_source;
	op_ret = tuya_create_led_handle(D3, &gnd_source);
	if (op_ret != OPRT_OK) {
		return op_ret;
	}
	tuya_set_led_type(gnd_source, OL_LOW, 0);

	op_ret = tuya_kb_init();
	if (op_ret != OPRT_OK) {
		return op_ret;
	}

	op_ret = tuya_kb_reg_proc(BUTTON_PIN, PRESS_HOLD_TIME, key_process);
	//PIN_PULLUP_EN(BUTTON_PIN_PERIPHS)

    /* wifi状态监测定时器 wifi状态改变时触发相关操作 */
    op_ret = sys_add_timer(wfl_timer_cb, NULL, &wfl_timer);
    if(OPRT_OK != op_ret) {
        return op_ret;
    }else {
        sys_start_timer(wfl_timer, 300, TIMER_CYCLE);
    }

	return op_ret;
}

STATIC VOID apply_state() {
    tuya_set_led_type(state_led_handle, state ? OL_LOW : OL_HIGH, 0);
    tuya_set_led_type(relay_handle, state ? OL_HIGH : OL_LOW, 0);		
}

/**
 * @brief 按键回调
 * 
 * @param gpio_no 按键触发引脚号
 * @param type 按下类型（长按、单击）
 * @param cnt 连续操作计数值
 * @return STATIC key_process 
 */
STATIC VOID key_process(INT gpio_no, PUSH_KEY_TYPE_E type, INT cnt) {
	PR_DEBUG("gpio_no: %d", gpio_no);
	PR_DEBUG("type: %d", type);
	PR_DEBUG("cnt: %d", cnt);

	if(gpio_no == BUTTON_PIN) {
        if (LONG_KEY == type) {
            tuya_dev_reset_factory();
        } else if (NORMAL_KEY == type) {
        	state = !state;
        	apply_state();
            all_channel_stat_upload();
        }

	}
}
 
VOID hw_set_wifi_led_stat(GW_WIFI_STAT_E wifi_stat) {
	switch (wifi_stat) {
		case STAT_UNPROVISION: {
			tuya_set_led_type(wifi_stat_led_handle, OL_FLASH_HIGH, 250);
		} break;

		case STAT_AP_STA_UNCONN:
		case STAT_AP_STA_CONN: {
			tuya_set_led_type(wifi_stat_led_handle, OL_FLASH_HIGH, 1500);
		} break;

		case STAT_LOW_POWER:
			break;

		case STAT_STA_UNCONN: {
			tuya_set_led_type(wifi_stat_led_handle, OL_HIGH, 0);
		} break;

		case STAT_STA_CONN: {
			tuya_set_led_type(wifi_stat_led_handle, OL_LOW, 0);
		} break;
	}
}


/**
 * @brief wifi状态监控定时回调
 * 
 * @param timerID 定时器ID
 * @param pTimerArg 
 * @return STATIC wfl_timer_cb 
 */
STATIC VOID wfl_timer_cb(UINT timerID, PVOID pTimerArg) {
	STATIC UINT last_wf_stat = 0xffffffff;
	STATIC BOOL is_syn = FALSE;

	GW_WIFI_STAT_E wf_stat = tuya_get_wf_status();
	if (last_wf_stat != wf_stat) {
		PR_DEBUG("wf_stat:%d", wf_stat);
		hw_set_wifi_led_stat(wf_stat);
		last_wf_stat = wf_stat;
		
		/* 设备离线/同步标志清除 */
		if (wf_stat == STAT_STA_UNCONN && is_syn) {
			is_syn = FALSE;
		}
	}

	/* 等待系统正常工作/同步所有状态 */
	if (is_syn == FALSE && (STAT_WORK == tuya_get_gw_status())) {
		all_channel_stat_upload();
		is_syn = TRUE;
	}
}

VOID device_cb(SMART_CMD_E cmd, cJSON *root) {
	PR_DEBUG("cmd:%d", cmd);

	cJSON *nxt = root->child;
	UCHAR dpid = 0;
	UINT dps = 0;

	while (nxt) {
		dpid = atoi(nxt->string);
		if (dpid == DPID) {
			state = (nxt->type == cJSON_True);
			apply_state();
			break;
		}
		nxt = nxt->next;
	}

	all_channel_stat_upload();
}

/**
 * @brief 系统所有状态上报
 * 
 * @return 
 */
STATIC VOID all_channel_stat_upload(VOID) {
	cJSON *root = NULL;
	root = cJSON_CreateObject();
	if (NULL == root) {
		PR_ERR("C_JSON_ERROR");
		return;
	}

	if (state) {
		cJSON_AddTrueToObject(root, DPIDS);
	} else {
		cJSON_AddFalseToObject(root, DPIDS);
	}

	CHAR *out = cJSON_PrintUnformatted(root);
	if (NULL == out) {
		PR_ERR("OUT = NULL");
		cJSON_Delete(root);
		return;
	}
	/* 查看上报信息 */
	PR_DEBUG("OUT = %s ", out);

	tuya_obj_dp_report(out);
	Free(out);
	cJSON_Delete(root);
	
	return;
}
