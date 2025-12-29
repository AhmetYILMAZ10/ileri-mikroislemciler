#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"

// Pin tanýmlarý
#define LCD_RS GPIO_PIN_1   // PE1
#define LCD_RW GPIO_PIN_2   // PE2
#define LCD_EN GPIO_PIN_3   // PE3
#define LCD_CTRL_PORT GPIO_PORTE_BASE

#define LCD_DATA_PORT GPIO_PORTB_BASE
#define D4 GPIO_PIN_4
#define D5 GPIO_PIN_5
#define D6 GPIO_PIN_6
#define D7 GPIO_PIN_7

void lcd_init(void);
void lcd_komut(uint8_t cmd);
void lcd_yaz(char *str);
void lcd_gotoxy(uint8_t satir, uint8_t sutun);
static void lcd_nibble(uint8_t data);

void lcd_init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinTypeGPIOOutput(LCD_CTRL_PORT, LCD_RS | LCD_RW | LCD_EN);
    GPIOPinTypeGPIOOutput(LCD_DATA_PORT, D4 | D5 | D6 | D7);

    GPIOPinWrite(LCD_CTRL_PORT, LCD_RW, 0); // her zaman yazma modu

    SysCtlDelay(50000);

    lcd_komut(0x28); // 4-bit, 2 satýr, 5x8 font
    lcd_komut(0x0C); // Display ON, cursor OFF
    lcd_komut(0x06); // Auto-increment
    lcd_komut(0x01); // Clear
    SysCtlDelay(50000);
}

void lcd_komut(uint8_t cmd)
{
    GPIOPinWrite(LCD_CTRL_PORT, LCD_RS, 0);
    lcd_nibble(cmd >> 4);
    lcd_nibble(cmd & 0x0F);
    SysCtlDelay(2000);
}

void lcd_yaz(char *str)
{
    while (*str)
    {
        GPIOPinWrite(LCD_CTRL_PORT, LCD_RS, LCD_RS);
        lcd_nibble(*str >> 4);
        lcd_nibble(*str & 0x0F);
        str++;
        SysCtlDelay(2000);
    }
}

void lcd_gotoxy(uint8_t satir, uint8_t sutun)
{
    uint8_t adres = (satir == 1) ? 0x80 : 0xC0;
    adres += (sutun - 1);
    lcd_komut(adres);
}

static void lcd_nibble(uint8_t data)
{
    GPIOPinWrite(LCD_DATA_PORT, D4|D5|D6|D7, 0);

    if (data & 0x01) GPIOPinWrite(LCD_DATA_PORT, D4, D4);
    if (data & 0x02) GPIOPinWrite(LCD_DATA_PORT, D5, D5);
    if (data & 0x04) GPIOPinWrite(LCD_DATA_PORT, D6, D6);
    if (data & 0x08) GPIOPinWrite(LCD_DATA_PORT, D7, D7);

    GPIOPinWrite(LCD_CTRL_PORT, LCD_EN, LCD_EN);
    SysCtlDelay(1000);
    GPIOPinWrite(LCD_CTRL_PORT, LCD_EN, 0);
    SysCtlDelay(1000);
}

#endif

