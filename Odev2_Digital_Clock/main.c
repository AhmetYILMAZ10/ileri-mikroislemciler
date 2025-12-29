#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "lcd.h"

volatile int sn = 45, dk = 06, sa = 19;

void initmikro(void);
void timerkesme(void);
void saat_guncelle(void);

int main(void)
{
    initmikro(); // LCD ekranı kullanıma hazır hale getir
    lcd_init();  // LCD başlat

    while(1) // Sonsuz döngü
    {
        saat_guncelle();  // Her saniye LCD'de zamanı güncelle
    }
}

void initmikro(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
                   SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); // Sistem saatini 40 MHz hızına ayarlar

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); //E Portu çevre birimini aktif eder.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); //B Portu çevre birimini aktif eder.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //F Portu çevre birimini aktif eder.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); //TIMER0 Portu çevre birimini aktif eder.

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1); // F portunun 1. pinini (Kırmızı LED) çıkış olarak ayarlar.

    // Timer0A -> periyodik modda, 1 saniyede bir kesme
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC); // Timer0'ı periyodik (sürekli tekrarlayan) moda ayarlar.
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() - 1);// Timer süresini 1 saniye (saat hızı kadar) olarak ayarlar.

    TimerIntRegister(TIMER0_BASE, TIMER_A, timerkesme); // Timer kesmesi oluştuğunda 'timerkesme' fonksiyonuna gitmesini söyler.
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // Timer kesmesini ve genel kesme sistemini aktif eder.
    IntEnable(INT_TIMER0A);
    IntMasterEnable();

    TimerEnable(TIMER0_BASE, TIMER_A);// Zamanlayıcıyı başlatır.
}

void timerkesme(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);// Kesme bayrağını temizler (bir sonraki kesme için gerekli).

    sn++;
    if (sn == 60) { sn = 0; dk++; } // 60 saniye olduysa dakikayı artır.
    if (dk == 60) { dk = 0; sa++; } // 60 dakika olduysa saati artır.
    if (sa == 24) { sa = 0; }

    // PF1 LED yanıp sönsün
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,
                 (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1) ^ GPIO_PIN_1));// PF1 pinindeki LED'in durumunu tersine çevir (Yanıyorsa söner, sönükse yanar)
}

void saat_guncelle(void)
{
    static int eski_sn = -1; // Saniyenin bir önceki değerini tutar

    if (sn != eski_sn)  // Saniye değiştiyse sadece o zaman yaz
    {
        eski_sn = sn;
        lcd_gotoxy(2, 1);  // 🔹 2. satır, 1. sütundan başlasın

        char buffer[9]; // Yazıyı tutacak geçici bir metin kutusu (HH:MM:SS)
        sprintf(buffer, "%02d:%02d:%02d", sa, dk, sn); // Saat, dakika ve saniyeyi "00:00:00" formatına çevirir
        lcd_yaz(buffer); // Oluşturulan metni LCD'ye yaz
    }
}
