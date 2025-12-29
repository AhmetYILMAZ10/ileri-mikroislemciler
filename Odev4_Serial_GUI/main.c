#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"

#include "Lcd.h"

// ===== Bayraklar =====
volatile bool ekran_guncelle = false;
volatile bool adc_oku_flag   = false;
volatile bool uart_gonder    = false;

// ===== Saat =====
volatile uint8_t saat = 19, dakika = 19, saniye = 19;

// ===== ADC =====
uint32_t adc_degeri;
uint8_t sicaklik;   // 0–40 °C

char saat_buffer[10];
char pot_buffer[6];

// ===== UART <ABC> =====
static char lcd_temp_buf[4];
static uint8_t lcd_idx = 0;
static bool veri_aliyor_muyuz = false;

// ===== UART !SAAT =====
static char time_buf[12];
static uint8_t time_i = 0;
static bool time_mode = false;

// ===== Prototipler =====
void AyarlariYap(void);
void Timer0_ISR(void);
void ADC_Oku(void);
void UART0_Init(void);
void UART0_SendLine(const char *s);

// =================================================
// UART RX
// =================================================
static void UART_RX_Process(void)
{
    while (UARTCharsAvail(UART0_BASE)) // UART'ta okunacak veri varsa
    {
        char c = UARTCharGet(UART0_BASE); // Karakteri oku
        UARTCharPut(UART0_BASE, c); // // Gelen karakteri geri gönder (Yankı/Echo)

        // -------- <ABC> --------
        if (c == '<') // Kapatma işareti geldiyse
        {
            veri_aliyor_muyuz = true;
            lcd_idx = 0;
            continue;
        }

        if (veri_aliyor_muyuz)
        {
            if (c == '>')
            {
                veri_aliyor_muyuz = false;
                lcd_temp_buf[lcd_idx] = '\0';

                // 1. satır 13–16 temizle
                Lcd_Goto(1,13); Lcd_Puts(" ");
                Lcd_Goto(1,14); Lcd_Puts(" ");
                Lcd_Goto(1,15); Lcd_Puts(" ");
                Lcd_Goto(1,16); Lcd_Puts(" ");

                char tek[2] = {0};
                // Gelen 3 karakteri LCD'ye tek tek bas
                if (lcd_temp_buf[0])
                {
                    tek[0] = lcd_temp_buf[0];
                    Lcd_Goto(1,13);
                    Lcd_Puts(tek);
                }
                if (lcd_temp_buf[1])
                {
                    tek[0] = lcd_temp_buf[1];
                    Lcd_Goto(1,14);
                    Lcd_Puts(tek);
                }
                if (lcd_temp_buf[2])
                {
                    tek[0] = lcd_temp_buf[2];
                    Lcd_Goto(1,15);
                    Lcd_Puts(tek);
                }
                continue;
            }

            if (lcd_idx < 3)
                lcd_temp_buf[lcd_idx++] = c;// Maksimum 3 karakter al

            continue;
        }

        // -------- !12:34:56 --------
        if (c == '!')
        {
            time_mode = true;
            time_i = 0;
            continue;
        }

        if (time_mode)
        {
            if (c == '\r' || c == '\n')// Satır sonu geldiyse
            {
                time_buf[time_i] = '\0';
                time_mode = false;

                int h,m,s;
                if (sscanf(time_buf,"%d:%d:%d",&h,&m,&s)==3)// Metni HH:MM:SS olarak sayılara parçala
                {
                    if (h<24 && m<60 && s<60)
                    {
                        saat=h; dakika=m; saniye=s;
                        ekran_guncelle = true;
                    }
                }
                continue;
            }

            if (time_i < sizeof(time_buf)-1)
                time_buf[time_i++] = c;
        }
    }
}

// MAIN
int main(void)
{
    AyarlariYap(); // Tüm birimleri (UART, ADC, Timer) kur

    Lcd_init(); // LCD başlat
    Lcd_Temizle(); // Ekranı temizle

    while (1)
    {
        UART_RX_Process();

        // ===== BUTON PF4 =====
        if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) // Butona basıldıysa
        {
            SysCtlDelay(SysCtlClockGet()/3/100); // Ark sönümleme (Debounce)
            if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0)
            {
                UART0_SendLine("B1"); // Bilgisayara "B1" mesajı gönder
                SysCtlDelay(SysCtlClockGet()/3/2); // Uzun basmayı engelle
            }
        }

        // ===== ADC =====
        if (adc_oku_flag)
        {
            adc_oku_flag = false;
            ADC_Oku();

            sicaklik = (adc_degeri * 40) / 4095; // 0-40 dereceye ölçekle

            sprintf(pot_buffer,"%02uC",sicaklik);
            Lcd_Goto(2,13);
            Lcd_Puts(pot_buffer);// LCD sağ alta yaz
        }

        // ===== SAAT =====
        if (ekran_guncelle)
        {
            ekran_guncelle = false;
            sprintf(saat_buffer,"%02u:%02u:%02u",saat,dakika,saniye);
            Lcd_Goto(2,1);
            Lcd_Puts(saat_buffer); // LCD sol alta yaz
        }

        // ===== UART GÖNDER =====
        if (uart_gonder)
        {
            uart_gonder = false;

            char buf[32];
            sprintf(buf,"*%u",sicaklik);// Sıcaklığı gönder (*25)
            UART0_SendLine(buf);

            sprintf(buf,"#%02u:%02u:%02u",saat,dakika,saniye);// Saati gönder (#19:19:19)
            UART0_SendLine(buf);
        }
    }
}

//Kesme Fonksiyonu (TIMER)
void Timer0_ISR(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // Kesme bayrağını temizle

    saniye++;
    if (saniye==60){saniye=0; dakika++;}
    if (dakika==60){dakika=0; saat++;}
    if (saat==24){saat=0;}

    ekran_guncelle = true; // Döngüye "ekranı güncelle" komutu ver
    adc_oku_flag = true; // Döngüye "ADC oku" komutu ver
    uart_gonder = true; // Döngüye "UART gönder" komutu ver
}

//ADC Okuma İşlemi
void ADC_Oku(void)
{
    ADCProcessorTrigger(ADC0_BASE,3); // ADC'yi tetikle
    while(!ADCIntStatus(ADC0_BASE,3,false)){} // Bitene kadar bekle
    ADCIntClear(ADC0_BASE,3); // Bayrağı temizle
    ADCSequenceDataGet(ADC0_BASE,3,&adc_degeri); // Veriyi çek
}

//UART BAŞLATMA
void UART0_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // A portunu aç
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); // UART0'ı aç

    GPIOPinConfigure(GPIO_PA0_U0RX); // PA0'ı alıcı (RX) yap
    GPIOPinConfigure(GPIO_PA1_U0TX); // PA1'i verici (TX) yap
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(),115200,
        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);// 115200 Baud, 8-bit veri, 1-stop bit, paritesiz ayarla

    UARTEnable(UART0_BASE); // UART'ı başlat
}

void UART0_SendLine(const char *s)
{
    while(*s) UARTCharPut(UART0_BASE,*s++); // Metni karakter karakter gönder
    UARTCharPut(UART0_BASE,'\r'); // Satır başı
    UARTCharPut(UART0_BASE,'\n'); // Yeni satır
}

void AyarlariYap(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
                   SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);// Sistem saatini 40 MHz yap

    UART0_Init(); // UART kur

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);// F portu (Buton için) enerjilendirme
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // D portu (ADC için) enerjilendirme
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); // Timer0 enerjilendirmesi
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);  // ADC0 aç

    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);// Buton Ayarı (PF4 - Pull Up)

    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3); // PD3 AIN4

    ADCSequenceConfigure(ADC0_BASE,3,ADC_TRIGGER_PROCESSOR,0);
    ADCSequenceStepConfigure(ADC0_BASE,3,0,
        ADC_CTL_CH4 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE,3);
    // Timer Kesmesi Ayarı (1 saniyelik periyot)
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet());
    TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0_ISR);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);
}
