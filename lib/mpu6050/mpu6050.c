#include "mpu6050.h"

// Função para resetar e inicializar o MPU6050
void mpu6050_init()
{
    // Configura o I2C
    i2c_init(MPU_6050_I2C_PORT, 400 * 1000); // 400 kHz
    gpio_set_function(MPU_6050_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(MPU_6050_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(MPU_6050_I2C_SDA);
    gpio_pull_up(MPU_6050_I2C_SCL);

    bi_decl(bi_2pins_with_func(MPU_6050_I2C_SDA, MPU_6050_I2C_SCL, GPIO_FUNC_I2C));
    // Reseta e inicializa o MPU6050
    mpu6050_reset();
}


// Função para resetar e inicializar o MPU6050
void mpu6050_reset()
{
    // Dois bytes para reset: primeiro o registrador, segundo o dado
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(MPU_6050_I2C_PORT, MPU6050_ADDR, buf, 2, false);
    sleep_ms(100); // Aguarda reset e estabilização

    // Sai do modo sleep (registrador 0x6B, valor 0x00)
    buf[1] = 0x00;
    i2c_write_blocking(MPU_6050_I2C_PORT, MPU6050_ADDR, buf, 2, false);
    sleep_ms(10); // Aguarda estabilização após acordar
}

// Função para ler dados crus do acelerômetro, giroscópio e temperatura
void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp)
{
    uint8_t buffer[6];

    // Lê aceleração a partir do registrador 0x3B (6 bytes)
    uint8_t val = 0x3B;
    i2c_write_blocking(MPU_6050_I2C_PORT, MPU6050_ADDR, &val, 1, true);
    i2c_read_blocking(MPU_6050_I2C_PORT, MPU6050_ADDR, buffer, 6, false);

    for (int i = 0; i < 3; i++)
    {
        accel[i] = (buffer[i * 2] << 8) | buffer[(i * 2) + 1];
    }

    // Lê giroscópio a partir do registrador 0x43 (6 bytes)
    val = 0x43;
    i2c_write_blocking(MPU_6050_I2C_PORT, MPU6050_ADDR, &val, 1, true);
    i2c_read_blocking(MPU_6050_I2C_PORT, MPU6050_ADDR, buffer, 6, false);

    for (int i = 0; i < 3; i++)
    {
        gyro[i] = (buffer[i * 2] << 8) | buffer[(i * 2) + 1];
    }

    // Lê temperatura a partir do registrador 0x41 (2 bytes)
    val = 0x41;
    i2c_write_blocking(MPU_6050_I2C_PORT, MPU6050_ADDR, &val, 1, true);
    i2c_read_blocking(MPU_6050_I2C_PORT, MPU6050_ADDR, buffer, 2, false);

    *temp = (buffer[0] << 8) | buffer[1];
}
