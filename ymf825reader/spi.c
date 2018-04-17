#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spi.h"

#define IC_PIN RPI_BPLUS_GPIO_J8_15
#define PWM_PIN0 RPI_BPLUS_GPIO_J8_12
#define PWM_PIN1 RPI_BPLUS_GPIO_J8_35 
#define PWM_CHANNEL0 0
#define PWM_CHANNEL1 1

#define PWM_RANGE 8192
#define PWM_INCREMENT_VALUE 128
#define PWM_DECREMENT_VALUE 8

static int32_t pin_data[2] = { 0, 0 };

static void spi_initialize() {
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);
  bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);

  bcm2835_gpio_fsel(IC_PIN, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(PWM_PIN0, BCM2835_GPIO_FSEL_ALT5);
  bcm2835_gpio_fsel(PWM_PIN1, BCM2835_GPIO_FSEL_ALT5);

  bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);
  bcm2835_pwm_set_mode(PWM_CHANNEL0, 0, 1);
  bcm2835_pwm_set_mode(PWM_CHANNEL1, 0, 1);
  bcm2835_pwm_set_range(PWM_CHANNEL0, PWM_RANGE);
  bcm2835_pwm_set_range(PWM_CHANNEL1, PWM_RANGE);
}

void pwm_increment(uint8_t pin) {
  pin_data[pin] += PWM_INCREMENT_VALUE;

  if (pin_data[pin] >= PWM_RANGE)
    pin_data[pin] = PWM_RANGE - 1;
  
  //printf("%d\n", pin_data[pin]);
  bcm2835_pwm_set_data(pin == 0 ? PWM_CHANNEL0 :  PWM_CHANNEL1, pin_data[pin]);
}

void pwm_decrement() {
  pin_data[0] -= PWM_DECREMENT_VALUE;
  pin_data[1] -= PWM_DECREMENT_VALUE;

  if (pin_data[0] < 0)
    pin_data[0] = 0;

  if (pin_data[1] < 0)
    pin_data[1] = 0;

  bcm2835_pwm_set_data(PWM_CHANNEL0, pin_data[0]);
  bcm2835_pwm_set_data(PWM_CHANNEL1, pin_data[1]);
}

void spi_open() {
  if (!bcm2835_init()) {
    printf("failed to initialize bcm2835\n");
    exit(1);
  }

  bcm2835_spi_begin();
  spi_initialize();
}

void spi_write(uint8_t pin, uint8_t address, uint8_t data) {
  static char buffer[] = { 0x00, 0x00 };

  buffer[0] = address;
  buffer[1] = data;

  if (pin & 0x01) {
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_writenb(buffer, 2);
    pwm_increment(0);
  }

  if (pin & 0x02) {
    bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
    bcm2835_spi_writenb(buffer, 2);
    pwm_increment(1);
  }

  bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);
}

void spi_write_multiple(uint8_t pin, const uint8_t* address_data, size_t length) {
  size_t i;
  for (i = 0; i < length; i += 2)
    spi_write(pin, address_data[i + 0], address_data[i + 1]);
}

void spi_burst_write(uint8_t pin, uint8_t address, const uint8_t* data, size_t length) {
  static char buffer[512];

  buffer[0] = address;
  memcpy(buffer + 1, data, length);

  if (pin & 0x01) {
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_writenb(buffer, length + 1);
    pwm_increment(0);
  }

  if (pin & 0x02) {
    bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
    bcm2835_spi_writenb(buffer, length + 1);
    pwm_increment(1);
  }

  bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);
}

void spi_set_ic(bool value) {
  bcm2835_gpio_write(IC_PIN, value ? LOW : HIGH);
}

void spi_close() {
  bcm2835_pwm_set_data(PWM_CHANNEL0, 0);
  bcm2835_pwm_set_data(PWM_CHANNEL1, 0);
  bcm2835_spi_end();
  bcm2835_close();
}
