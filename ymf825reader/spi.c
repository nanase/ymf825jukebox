#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spi.h"

#define IC_PIN RPI_BPLUS_GPIO_J8_22

static void spi_initialize() {
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);
  bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);

  bcm2835_gpio_fsel(IC_PIN, BCM2835_GPIO_FSEL_OUTP);
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
  }

  if (pin & 0x02) {
    bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
    bcm2835_spi_writenb(buffer, 2);
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
  }

  if (pin & 0x02) {
    bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
    bcm2835_spi_writenb(buffer, length + 1);
  }

  bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);
}

void spi_set_ic(bool value) {
  bcm2835_gpio_write(IC_PIN, value ? LOW : HIGH);
}

void spi_close() {
  bcm2835_spi_end();
  bcm2835_close();
}
