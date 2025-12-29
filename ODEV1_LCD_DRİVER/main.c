#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "lcd.h"

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ); //Sistem saatini ayarlar: 16MHz harici kristal ve PLL kullanarak işlemciyi 40MHz hızına kurar
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //F Portu çevre birimini aktif eder.
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3); //F Portundaki 1, 2 ve 3. pinleri (RGB LED pinleri) ÇIKIŞ olarak ayarlar
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4); //F Portundaki 4. pini Butonu giriş olarak ayarlar
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU); //Buton için ayar:4mA akım gücü ve Dahili Pull-Up direnci (WPU) tanımlar
    baslangic();
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x02); //0000 1110 kırmızı ledi aktif eder

    satir_sutun(1,2); // LCD imlecini 1. satır, 2. sütun konumuna taşır
    printf("isim soyisim");
    satir_sutun(2,1); //LCD imlecini 2. satır, 1. sütun konumuna taşır
    printf("üniversite /okul no");
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x08); //0000 1110 yeşil ledi aktif eder
}



