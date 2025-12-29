#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

// ===============================================================
// LCD PIN TANIMLARI
// ===============================================================

// Kontrol pinleri (PORT E)
#define LCD_CTRL_PORT   GPIO_PORTE_BASE
#define LCD_RS          GPIO_PIN_1   // PE1
#define LCD_RW          GPIO_PIN_2   // PE2
#define LCD_EN          GPIO_PIN_3   // PE3

// Veri pinleri (PORT B)
#define LCD_DATA_PORT   GPIO_PORTB_BASE
#define D4              GPIO_PIN_4   // PB4
#define D5              GPIO_PIN_5   // PB5
#define D6              GPIO_PIN_6   // PB6
#define D7              GPIO_PIN_7   // PB7

// ===============================================================
// PROTOTIPLER
// ===============================================================
void lcd_init(void);
void lcd_komut(uint8_t cmd);
void lcd_yaz(char *str);
void lcd_gotoxy(uint8_t satir, uint8_t sutun);

static void lcd_nibble(uint8_t nibble);
static void lcd_pulse_enable(void);

// ===============================================================
// LCD BAŞLATMA
// ===============================================================
void lcd_init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinTypeGPIOOutput(LCD_CTRL_PORT, LCD_RS | LCD_RW | LCD_EN);
    GPIOPinTypeGPIOOutput(LCD_DATA_PORT, D4 | D5 | D6 | D7);

    GPIOPinWrite(LCD_CTRL_PORT, LCD_RW, 0);   // RW her zaman 0 (yazma modu)

    SysCtlDelay(50000); // LCD başlangıç gecikmesi

    // 4-bit mod başlangıç
    lcd_komut(0x28); // 4-bit, 2 satır
    lcd_komut(0x0C); // Display ON, cursor OFF
    lcd_komut(0x06); // Auto increment
    lcd_komut(0x01); // Clear
    SysCtlDelay(50000);
}

// ===============================================================
// LCD KOMUT GÖNDERME
// ===============================================================
void lcd_komut(uint8_t cmd)
{
    GPIOPinWrite(LCD_CTRL_PORT, LCD_RS, 0);  // RS = 0 → Komut

    lcd_nibble(cmd >> 4);  // üst nibble
    lcd_nibble(cmd & 0x0F); // alt nibble

    SysCtlDelay(2000);
}

// ===============================================================
// LCD YAZI YAZMA
// ===============================================================
void lcd_yaz(char *str)
{
    while (*str)
    {
        GPIOPinWrite(LCD_CTRL_PORT, LCD_RS, LCD_RS); // RS = 1 → Data

        lcd_nibble(*str >> 4);
        lcd_nibble(*str & 0x0F);

        str++;

        SysCtlDelay(2000);
    }
}

// ===============================================================
// İMLEÇ KONUMU AYARLAMA
// ===============================================================
void lcd_gotoxy(uint8_t satir, uint8_t sutun)
{
    uint8_t adres = (satir == 1) ? 0x80 : 0xC0;
    adres += (sutun - 1);
    lcd_komut(adres);
}

// ===============================================================
// TEK NİBBLE (4 bit) VERİ GÖNDERME
// ===============================================================
static void lcd_nibble(uint8_t nibble)
{
    // Önce tüm data pinlerini sıfırla
    GPIOPinWrite(LCD_DATA_PORT, D4 | D5 | D6 | D7, 0);

    // Hangi pin 1 olacaksa onu yaz
    if(nibble & 0x01) GPIOPinWrite(LCD_DATA_PORT, D4, D4);
    if(nibble & 0x02) GPIOPinWrite(LCD_DATA_PORT, D5, D5);
    if(nibble & 0x04) GPIOPinWrite(LCD_DATA_PORT, D6, D6);
    if(nibble & 0x08) GPIOPinWrite(LCD_DATA_PORT, D7, D7);

    lcd_pulse_enable();
}

// ===============================================================
// ENABLE PULSE
// ===============================================================
static void lcd_pulse_enable(void)
{
    GPIOPinWrite(LCD_CTRL_PORT, LCD_EN, LCD_EN);
    SysCtlDelay(1000);
    GPIOPinWrite(LCD_CTRL_PORT, LCD_EN, 0);
    SysCtlDelay(1000);
}
void Lcd_init(void)
{
    lcd_init();
}

void Lcd_Temizle(void)
{
    lcd_komut(0x01);
    SysCtlDelay(50000);
}

void Lcd_Goto(uint8_t satir, uint8_t sutun)
{
    lcd_gotoxy(satir, sutun);
}

void Lcd_Puts(char *str)
{
    lcd_yaz(str);
}
#endif
