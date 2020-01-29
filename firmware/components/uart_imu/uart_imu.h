#ifndef UART_IMU_H
#define UART_IMU_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"

int imu_init();
int parse_IMU_data();
void print_imu();

uint16_t get_acc_x_in_D16QN();
uint16_t get_acc_y_in_D16QN();
uint16_t get_acc_z_in_D16QN();

uint16_t get_gyr_x_in_D16QN();
uint16_t get_gyr_y_in_D16QN();
uint16_t get_gyr_z_in_D16QN();

uint16_t get_roll_in_D16QN();
uint16_t get_pitch_in_D16QN();
uint16_t get_yaw_in_D16QN();

uint16_t get_linacc_x_in_D16QN();
uint16_t get_linacc_y_in_D16QN();
uint16_t get_linacc_z_in_D16QN();

#endif


