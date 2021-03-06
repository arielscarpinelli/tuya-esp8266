#ifndef _TUYA_GPIO_API_H
#define _TUYA_GPIO_API_H

#include "com_def.h"

/*************************************************************************************
函数功能: 读取GPIO电平
输入参数: gpio_no:0-16分别对应IO0-IO16
输出参数: 无
返 回 值: 0 低电平 1 高电平
备    注: 无
*************************************************************************************/
INT tuya_read_gpio_level(USHORT gpio_no);

/*************************************************************************************
函数功能: 写入GPIO电平
输入参数: gpio_no:0-16分别对应IO0-IO16
		  level   GPIO电平
		  <1> 0 低电平
		  <2> 1 高电平
输出参数: 无
返 回 值: 无
备    注: 无
*************************************************************************************/
VOID tuya_write_gpio_level(USHORT gpio_no, UCHAR level);

/*************************************************************************************
函数功能: tuya sdk 在pre_app_int后app_init前初始化所有gpio为输入 如果想快速启动gpio，
		  可以在pre_app_int中启动该api，初始化为输出，后面不在初始化引脚为输入，达到快速启动
输入参数: gpio_info_list:初始化为输出的gpio数组
		  gpio_num   gpio个数
输出参数: 无
返 回 值: 无
备    注: 无
*************************************************************************************/
VOID tuya_pre_app_set_gpio_out(UCHAR gpio_info_list[], UCHAR gpio_num);


#endif
