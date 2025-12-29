#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> // sprintf için gerekli
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h" // ADC kütüphanesi
#include "lcd.h"

// Saat değişkenleri
volatile int sn =10 , dk = 17 , sa = 19;
volatile int sicaklik = 0; // Sıcaklık değerini tutacak

// Global tanımlama: ADC'nin bağlandığı pin
// PD3 (AIN4) kullanılıyor
#define ADC_KANALI 4 // AIN4 = PD3

// Fonksiyon prototipleri
void initmikro(void);
void init_adc(void);
void timerkesme(void);
void saat_guncelle(void);
void sicaklik_oku_goster(void);

int main(void)
{
    initmikro(); // Donanımı hazırla
    init_adc(); // ADC'yi başlat
    lcd_init(); // LCD başlat

    // 1. Satıra Ad Soyad yazma
    lcd_gotoxy(1, 1); // 1. satır, 1. sütun
    lcd_yaz("AD SOYADI"); // BURAYA ADINI YAZ

    while(1)
    {
        saat_guncelle();       // Her saniye LCD'de zamanı güncelle
        sicaklik_oku_goster(); // Döngüde sürekli sıcaklığı güncelle
    }
}

//  Başlatma Fonksiyonları

void initmikro(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);// 40 MHz Saat ayarı

    // Gerekli Çevre Birimlerini Etkinleştir
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); // LCD kontrol (PE1, PE2, PE3)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // LCD veri (PB4-PB7)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // LED (PF1)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // 🆕 ADC (PD3) için eklendi
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);// PF1 (LED) çıkış olarak ayarla

    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);// Timer0A -> periyodik modda, 1 saniyede bir kesme
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() - 1); // 40Mhz'de 1 saniye
    // Kesme fonksiyonunu kaydeder ve sistemi aktif eder
    TimerIntRegister(TIMER0_BASE, TIMER_A, timerkesme);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);
    IntMasterEnable();

    TimerEnable(TIMER0_BASE, TIMER_A); // Zamanlayıcıyı başlat
}

void init_adc(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // ADC0 modülünü etkinleştir

    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3);//  PD3'ü (AIN4) analog giriş olarak ayarla

    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);// Sequencer 3: İşlemci tetiklemeli, öncelik 0

    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_KANALI | ADC_CTL_IE | ADC_CTL_END);// Sequencer 3, ilk adım: ADC_KANALI'ndan oku, kesme üret ve işlemi bitir.

    ADCSequenceEnable(ADC0_BASE, 3);// Adc dizisi Sequencer 3'ü etkinleştir
}

// Kesme ve Saat Fonksiyonları
void timerkesme(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // Kesme bayrağını temizle

    // Saati ilerlet (Dakika ve saat geçişlerini kontrol et)
    sn++;
    if (sn == 60) { sn = 0; dk++; }
    if (dk == 60) { dk = 0; sa++; }
    if (sa == 24) { sa = 0; }

    // PF1 LED yanıp sönsün
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1,
   (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1) ^ GPIO_PIN_1));
}
void saat_guncelle(void)
{
    static int eski_sn = -1;// Sadece saniye değiştiğinde ekranı güncelle (titremeyi önler)

    if (sn != eski_sn) // Saniye değiştiyse sadece o zaman yaz
    {
        eski_sn = sn;
        lcd_gotoxy(2, 1); // 2. satır, 1. sütun (Saatin başlangıcı)

        char buffer[9];
        sprintf(buffer, "%02d:%02d:%02d", sa, dk, sn); //"19:17:10" formatına getir
        lcd_yaz(buffer); // LCD'ye bas
    }
}
// Sıcaklık Okuma Fonksiyonu
void sicaklik_oku_goster(void)
{
    uint32_t adc_degeri[1];

    ADCProcessorTrigger(ADC0_BASE, 3); // İşlemci tetikleyicisiyle ADC okumasını başlat

    while(!ADCIntStatus(ADC0_BASE, 3, false)){} // Okuma bitene kadar bekle

    ADCIntClear(ADC0_BASE, 3);// Kesme bayrağını temizle

    ADCSequenceDataGet(ADC0_BASE, 3, adc_degeri); // Okunan değeri al

    uint32_t raw_adc = adc_degeri[0];

    // Lineer dönüşüm: Sicaklik = (raw_adc / 4095) * 40
    // (0-4095 aralığı 0-40 dereceye eşlenir)
    sicaklik = (raw_adc * 40) / 4095; // ADC verisini (0-4095) 0-40 derece arasına dönüştürür

    // LCD'de sıcaklık değerini yaz
    lcd_gotoxy(2, 11); // 2. satır, 11. sütun (Saatin hemen yanına)

    char buffer[7];
    sprintf(buffer, " %02dC", sicaklik);
    lcd_yaz(buffer);
}
