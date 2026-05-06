#include "ti_msp_dl_config.h"

/*You can define your own global variables here, the three variables below are used as an example which could be useful for this task*/
volatile int32_t bufferSample[1000]={0};     //the ring buffer to store the (squared) value converted by ADC
volatile int32_t sampleIndex=0;     //show at which position in the ring buffer should the current value be stored
volatile int32_t movingAverageOutput = 0;      //the calculated moving average result
volatile int32_t runningSum = 0;           // Speichert die fortlaufende Summe der letzten 1000 Werte
volatile int64_t dcFilterSum = 67108864LL; // Langsamer Tiefpassfilter für Auto-Kalibrierung des DC-Offsets

int main(void)   //DO NOT CHANGE THE main() FUNCTION!
{
    SYSCFG_DL_init();  //initialize the system configurations
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);  //enable interrupt for the ADC

    while (1) {  //endless loop which can be interrupted by the ADC12_0_INST_IRQHandler() function below
    }
}

void ADC12_0_INST_IRQHandler(void)  //DO NOT CHANGE THE EXISTING PART OF CODE IN THIS FUNCTION, ONLY ADD YOUR CODE AT THE INDICATED POSITION
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST)) {  //check which kind of interrupt was triggered
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:  //interrupt caused by the finished and stored ADC conversion result
            DL_TimerA_stopCounter(TIMER_0_INST);  //temporarily stop the timer to avoid possible collision
            
            /*write your own code here, temporary variables can be defined here if needed*/

            // 1. ADC-Wert auslesen (Wertebereich 0 bis 4095)
            int32_t rawADC = DL_ADC12_getMemResult(ADC12_0_INST, DL_ADC12_MEM_IDX_0);

            // 2. AUTO-KALIBRIERUNG (DC-Offset entfernen)
            // Tiefpassfilterung des Klinkenrauschens
            dcFilterSum = dcFilterSum - (dcFilterSum / 32768) + rawADC;
            int32_t currentDC = (int32_t)(dcFilterSum / 32768);
            int32_t acSignal = rawADC - currentDC;

            // 3. Skalieren und Quadrieren
            int32_t scaled = acSignal / 16;  // Herunterskalieren, um ein Überlaufen der 32-Bit Summe zu verhindern
            int32_t squaredValue = scaled * scaled; // Quadrieren berechnet die Signalenergie

            // 4. RINGPUFFER & INKREMENTELLER MOVING AVERAGE (Gleitender Mittelwert)
            // Anstatt in jedem Schritt 1000 Werte per Schleife neu zu addieren (was viel 
            // Rechenzeit kostet), passen wir nur die Differenz an.
                        
            // a) Subtraktion: Ziehe den ältesten quadrierten Wert, der jetzt aus dem "Fenster" fällt, von der Gesamtsumme ab.
            runningSum -= bufferSample[sampleIndex];
            
            // b) Addition: Addiere den neuen quadrierten Wert zur Gesamtsumme.
            runningSum += squaredValue;
            
            // c) Speichern: Überschreibe den alten Wert im Array mit dem neuen Wert.
            bufferSample[sampleIndex] = squaredValue;
            
            // d) Mittelwert bilden: Teile die neue Summe durch die Fenstergröße N=1000.
            movingAverageOutput = (int32_t)(runningSum / 1000);
            
            // e) Index-Verwaltung (Wrap-around): Erhöhe den Index. Der Modulo-Operator (% 1000) 
            // sorgt dafür, dass der Index nach 999 wieder auf 0 springt. Der Puffer läuft im Kreis.
            sampleIndex = (sampleIndex + 1) % 1000;

            // 5. LED-Ansteuerung (Logarithmische Pegelanzeige)
            uint32_t ledMask =
                GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN |
                GPIO_GRP_0_PIN_4_PIN | GPIO_GRP_0_PIN_5_PIN |
                GPIO_GRP_0_PIN_6_PIN | GPIO_GRP_0_PIN_7_PIN;

            uint32_t ledValue = 0;

            // Schwellenwerte fangen Rauschen (<15) ab und steigen dann exponentiell an
            if (movingAverageOutput > 1500)
                ledValue = ledMask; // 8 LEDs
            else if (movingAverageOutput > 1000)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN |
                           GPIO_GRP_0_PIN_4_PIN | GPIO_GRP_0_PIN_5_PIN |
                           GPIO_GRP_0_PIN_6_PIN; // 7 LEDs
            else if (movingAverageOutput > 600)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN |
                           GPIO_GRP_0_PIN_4_PIN | GPIO_GRP_0_PIN_5_PIN; // 6 LEDs
            else if (movingAverageOutput > 300)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN |
                           GPIO_GRP_0_PIN_4_PIN; // 5 LEDs
            else if (movingAverageOutput > 150)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN; // 4 LEDs
            else if (movingAverageOutput > 80)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN; // 3 LEDs
            else if (movingAverageOutput > 40)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN; // 2 LEDs
            else if (movingAverageOutput > 15)
                ledValue = GPIO_GRP_0_PIN_0_PIN; // 1 LED 
            else
                ledValue = 0x000; // 0 LEDs 

            DL_GPIO_writePinsVal(GPIO_GRP_0_PORT, ledMask, ledValue);

            /*end of your code*/
            
            DL_TimerA_startCounter(TIMER_0_INST);  //start the timer again for the next ADC conversion
            break;
        default:
            break;
    }
}

/*You can also define extra functions here below if needed*/