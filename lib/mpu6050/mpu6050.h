#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

// Definição dos pinos I2C para o MPU6050
#define MPU_6050_I2C_PORT i2c0                 // I2C0 usa pinos 0 e 1
#define MPU_6050_I2C_SDA 0
#define MPU_6050_I2C_SCL 1

// Endereço I2C do MPU6050
#define MPU6050_ADDR 0x68

// Função para resetar e inicializar o MPU6050
void mpu6050_init();

// Função para resetar e inicializar o MPU6050
void mpu6050_reset();

// Função para ler dados crus do acelerômetro, giroscópio e temperatura
void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp);


#endif // MPU6050_H
