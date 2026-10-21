/* led.c
 *
 * Test bare-metal blinking led application
 *
 * Copyright (C) 2026 wolfSSL Inc.
 *
 * This file is part of wolfBoot.
 *
 * wolfBoot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfBoot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */
#include <stdint.h>
#include "wolfboot/wolfboot.h"

#ifdef TARGET_stm32f4
/* ALIENTEK Explorer (STM32F407ZGT6): DS0=PF9 (red), DS1=PF10 (green),
 * both active-low (drive the pin low to light). Configured as plain GPIO
 * outputs so the blink is visible right after wolfBoot hands control to
 * the APP, independent of the TIM4_CH4/PD15 PWM path used by pwm_init()
 * (PD15 is not wired to a LED on this board). */
#define AHB1_CLOCK_ER         (*(volatile uint32_t *)(0x40023830)) /* RCC_AHB1ENR */
#define GPIOF_AHB1_CLOCK_ER   (1u << 5)                           /* GPIOFEN */

#define GPIOF_BASE          (0x40021400)
#define GPIOF_MODE          (*(volatile uint32_t *)(GPIOF_BASE + 0x00))
#define GPIOF_OTYPE         (*(volatile uint32_t *)(GPIOF_BASE + 0x04))
#define GPIOF_PUPD          (*(volatile uint32_t *)(GPIOF_BASE + 0x0c))
#define GPIOF_ODR           (*(volatile uint32_t *)(GPIOF_BASE + 0x14))
#define GPIOF_BSRR          (*(volatile uint32_t *)(GPIOF_BASE + 0x18))

#define LED0_PIN            (9)  /* PF9  - DS0 (red)   */
#define LED1_PIN            (10) /* PF10 - DS1 (green) */

static void led_pins_init(void)
{
    uint32_t reg;
    AHB1_CLOCK_ER |= GPIOF_AHB1_CLOCK_ER;
    (void)AHB1_CLOCK_ER; /* read-back so GPIOF clock is live before MODER */
    /* PF9/PF10 -> general purpose output (01), push-pull, no pull */
    reg = GPIOF_MODE & ~((0x03u << (LED0_PIN * 2)) | (0x03u << (LED1_PIN * 2)));
    GPIOF_MODE = reg | (1u << (LED0_PIN * 2)) | (1u << (LED1_PIN * 2));
    GPIOF_OTYPE &= ~((1u << LED0_PIN) | (1u << LED1_PIN));
    reg = GPIOF_PUPD & ~((0x03u << (LED0_PIN * 2)) | (0x03u << (LED1_PIN * 2)));
    GPIOF_PUPD = reg;
}

void led_pwm_setup(void)
{
    led_pins_init();
    /* active-low: clear PF9/PF10 to light DS0/DS1 */
    GPIOF_BSRR = (1u << (LED1_PIN + 16)) | (1u << (LED0_PIN + 16));
}

void boot_led_on(void)
{
    led_pins_init();
    GPIOF_BSRR = (1u << (LED1_PIN + 16)) | (1u << (LED0_PIN + 16));
}

void boot_led_off(void)
{
    GPIOF_BSRR = (1u << LED1_PIN) | (1u << LED0_PIN);
}

void led_on(void)
{
    GPIOF_BSRR = (1u << (LED1_PIN + 16)) | (1u << (LED0_PIN + 16));
}

void led_off(void)
{
    GPIOF_BSRR = (1u << LED1_PIN) | (1u << LED0_PIN);
}

void led_toggle(void)
{
    /* Toggle both DS0/DS1 together. ODR read/modify/write is safe here
     * because main() never touches GPIOF_ODR after setup. */
    GPIOF_ODR ^= (1u << LED0_PIN) | (1u << LED1_PIN);
}
#endif /* TARGET_stm32f4 */

#ifdef TARGET_stm32l0
#define LED_BOOT_PIN (5)

#define RCC_IOPENR (*(volatile uint32_t *)(0x4002102C))
#define IOPAEN (1 << 0)


#define GPIOA_BASE 0x50000000
#define GPIOA_MODE  (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_OTYPE (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_OSPD  (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOA_PUPD  (*(volatile uint32_t *)(GPIOA_BASE + 0x0c))
#define GPIOA_ODR   (*(volatile uint32_t *)(GPIOA_BASE + 0x14))
#define GPIOA_BSRR  (*(volatile uint32_t *)(GPIOA_BASE + 0x18))
#define GPIOA_AFL   (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
#define GPIOA_AFH   (*(volatile uint32_t *)(GPIOA_BASE + 0x24))


void boot_led_on(void)
{
    uint32_t reg;
    uint32_t pin = LED_BOOT_PIN;
    RCC_IOPENR |= IOPAEN;
    reg = GPIOA_MODE & ~(0x03 << (pin * 2));
    GPIOA_MODE = reg | (1 << (pin * 2));
    reg = GPIOA_PUPD & ~(0x03 << (pin * 2));
    GPIOA_PUPD = reg | (1 << (pin * 2));
    GPIOA_BSRR |= (1 << pin);
}

void boot_led_off(void)
{
    uint32_t pin = LED_BOOT_PIN;
    GPIOA_BSRR |= (1 << (pin + 16));
}

#endif /* TARGET_stm32l0 */

#if defined(TARGET_stm32g0) || defined(TARGET_stm32c0)
/* GPIOA5 */
#define RCC_IOPENR (*(volatile uint32_t *)(0x40021034))
#define RCC_IOPENR_GPIOAEN (1 << 0)

#define GPIOA_BASE 0x50000000
#define GPIOA_MODE  (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_OTYPE (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_OSPD  (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOA_PUPD  (*(volatile uint32_t *)(GPIOA_BASE + 0x0c))
#define GPIOA_ODR   (*(volatile uint32_t *)(GPIOA_BASE + 0x14))
#define GPIOA_BSRR  (*(volatile uint32_t *)(GPIOA_BASE + 0x18))
#define GPIOA_AFL   (*(volatile uint32_t *)(GPIOA_BASE + 0x20))
#define GPIOA_AFH   (*(volatile uint32_t *)(GPIOA_BASE + 0x24))
#define LED_PIN (5)
#define LED_BOOT_PIN (5)
#define GPIO_OSPEED_100MHZ (0x03)

void boot_led_on(void)
{
    uint32_t reg;
    uint32_t pin = LED_BOOT_PIN;
    RCC_IOPENR |= RCC_IOPENR_GPIOAEN;
    reg = GPIOA_MODE & ~(0x03 << (pin * 2));
    GPIOA_MODE = reg | (1 << (pin * 2)); /* general purpose output mode */
    reg = GPIOA_PUPD & ~(0x03 << (pin * 2));
    GPIOA_PUPD = reg | (1 << (pin * 2)); /* pull-up */
    GPIOA_BSRR |= (1 << pin); /* set pin */
}

#endif /* TARGET_stm32g0 || TARGET_stm32c0 */

#ifdef TARGET_stm32wb
#define LED_BOOT_PIN (0)
#define RCC_AHB2_CLOCK_ER (*(volatile uint32_t *)(0x5800004C))
#define GPIOB_AHB2_CLOCK_ER (1 << 1)

#define GPIOB_BASE 0x48000400
#define GPIOB_MODE  (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPE (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_OSPD  (*(volatile uint32_t *)(GPIOB_BASE + 0x08))
#define GPIOB_PUPD  (*(volatile uint32_t *)(GPIOB_BASE + 0x0c))
#define GPIOB_ODR   (*(volatile uint32_t *)(GPIOB_BASE + 0x14))
#define GPIOB_BSRR  (*(volatile uint32_t *)(GPIOB_BASE + 0x18))
#define GPIOB_AFL   (*(volatile uint32_t *)(GPIOB_BASE + 0x20))
#define GPIOB_AFH   (*(volatile uint32_t *)(GPIOB_BASE + 0x24))


void boot_led_on(void)
{
    uint32_t reg;
    uint32_t pin = LED_BOOT_PIN;
    RCC_AHB2_CLOCK_ER |= GPIOB_AHB2_CLOCK_ER;
    reg = GPIOB_MODE & ~(0x03 << (pin * 2));
    GPIOB_MODE = reg | (1 << (pin * 2));
    reg = GPIOB_PUPD & ~(0x03 << (pin * 2));
    GPIOB_PUPD = reg | (1 << (pin * 2));
    GPIOB_BSRR |= (1 << pin);
}

void boot_led_off(void)
{
    uint32_t reg;
    uint32_t pin = LED_BOOT_PIN;
    GPIOB_BSRR |= (1 << (pin + 16));
}


#endif /* TARGET_stm32wb */

#ifdef TARGET_stm32l4
#define AHB2_CLOCK_ER (*(volatile uint32_t *)(0x4002104C)) /* RCC_AHB2ENR */
#define GPIOB_AHB2_CLOCK_ER (1 << 1)

#define GPIOB_BASE 0x48000400
#define GPIOB_MODE  (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPE (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_OSPD  (*(volatile uint32_t *)(GPIOB_BASE + 0x08))
#define GPIOB_PUPD  (*(volatile uint32_t *)(GPIOB_BASE + 0x0c))
#define GPIOB_ODR   (*(volatile uint32_t *)(GPIOB_BASE + 0x14))
#define GPIOB_BSRR  (*(volatile uint32_t *)(GPIOB_BASE + 0x18))
#define GPIOB_AFL   (*(volatile uint32_t *)(GPIOB_BASE + 0x20))
#define GPIOB_AFH   (*(volatile uint32_t *)(GPIOB_BASE + 0x24))
#define LED_PIN (14) /* User LD3: a red user LED is connected to PB14 */
#define LED_BOOT_PIN (7) /* User LD2: a blue user LED is connected to PB7 */
#define GPIO_OSPEED_100MHZ (0x03)


void boot_led_on(void)
{
    uint32_t reg;
    uint32_t pin = LED_BOOT_PIN;
    AHB2_CLOCK_ER |= GPIOB_AHB2_CLOCK_ER;
    reg = GPIOB_MODE & ~(0x03 << (pin * 2));
    GPIOB_MODE = reg | (1 << (pin * 2));
    reg = GPIOB_PUPD & ~(0x03 << (pin * 2));
    GPIOB_PUPD = reg | (1 << (pin * 2));
    GPIOB_BSRR |= (1 << pin);
}
void led_on(void)
{
    uint32_t reg;
    uint32_t pin = LED_PIN;
    AHB2_CLOCK_ER |= GPIOB_AHB2_CLOCK_ER;
    reg = GPIOB_MODE & ~(0x03 << (pin * 2));
    GPIOB_MODE = reg | (1 << (pin * 2));
    reg = GPIOB_PUPD & ~(0x03 << (pin * 2));
    GPIOB_PUPD = reg | (1 << (pin * 2));
    GPIOB_BSRR |= (1 << pin);
}
void led_off(void)
{
    GPIOB_BSRR |= (1 << (LED_PIN + 16));
}
void boot_led_off(void)
{
    GPIOB_BSRR |= (1 << (LED_BOOT_PIN + 16));
}

#endif /* TARGET_stm32l4 */

#ifdef TARGET_stm32g4
#include "stm32g4.h"
/* NUCLEO-G491RE user LED LD2 on PA5. */

void boot_led_on(void)
{
    uint32_t reg;
    const uint32_t pin = NUCLEO_G491_LED_LD2_PIN;
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    reg = RCC_AHB2ENR; /* read-back so GPIOA clock is live before MODER write */
    (void)reg;
    reg = GPIOA_MODER & ~(0x03 << (pin * 2));
    GPIOA_MODER = reg | (1 << (pin * 2));
    reg = GPIOA_PUPDR & ~(0x03 << (pin * 2));
    GPIOA_PUPDR = reg | (1 << (pin * 2));
    GPIOA_BSRR = (1 << pin); /* BSRR is write-only; bare assignment, not |= */
}

void boot_led_off(void)
{
    GPIOA_BSRR = (1 << (NUCLEO_G491_LED_LD2_PIN + 16));
}

#endif /* TARGET_stm32g4 */
