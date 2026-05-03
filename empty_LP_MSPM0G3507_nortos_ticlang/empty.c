#include "ti_msp_dl_config.h"

volatile int32_t bufferSample[1000] = {0};
volatile int32_t sampleIndex = 0;
volatile int32_t movingAverageOutput = 0;
volatile int64_t runningSum = 0;  // int64_t wegen Überlauf!

int main(void)
{
    SYSCFG_DL_init();
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
    while (1) {}
}

void ADC12_0_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            DL_TimerA_stopCounter(TIMER_0_INST);

            // 1. ADC-Wert lesen (0..4095, kein Offset!)
            int32_t adcResult =
                DL_ADC12_getMemResult(ADC12_0_INST, DL_ADC12_MEM_IDX_0);

            // 2. Quadrieren und Ringpuffer aktualisieren
            // Skalierung /16 VOR dem Quadrieren reduziert Wertebereich:
            // 4095/16 = 255, 255² = 65025, ×1000 = 65 Mio → passt in int32_t
            int32_t scaled = adcResult >> 4;  // /16 per Bitshift
            int32_t squaredValue = scaled * scaled;

            runningSum -= bufferSample[sampleIndex];
            runningSum += squaredValue;
            bufferSample[sampleIndex] = squaredValue;

            movingAverageOutput = (int32_t)(runningSum / 1000);
            sampleIndex = (sampleIndex + 1) % 1000;

            // 3. LEDs setzen
            // Alle 8 LED-Pins als Maske
            uint32_t ledMask =
                GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN |
                GPIO_GRP_0_PIN_4_PIN | GPIO_GRP_0_PIN_5_PIN |
                GPIO_GRP_0_PIN_6_PIN | GPIO_GRP_0_PIN_7_PIN;

            uint32_t ledValue = 0;

            // Schwellenwerte für skalierte Werte (scaled = adcResult/16)
            // Bei leiser Musik: kleine Werte, bei lauter: größere
            if      (movingAverageOutput > 1500) ledValue = ledMask;
            else if (movingAverageOutput > 700)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN |
                           GPIO_GRP_0_PIN_4_PIN | GPIO_GRP_0_PIN_5_PIN |
                           GPIO_GRP_0_PIN_6_PIN;
            else if (movingAverageOutput > 450)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN |
                           GPIO_GRP_0_PIN_4_PIN | GPIO_GRP_0_PIN_5_PIN;
            else if (movingAverageOutput > 300)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN |
                           GPIO_GRP_0_PIN_4_PIN;
            else if (movingAverageOutput > 200)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN | GPIO_GRP_0_PIN_3_PIN;
            else if (movingAverageOutput > 100)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN |
                           GPIO_GRP_0_PIN_2_PIN;
            else if (movingAverageOutput > 80)
                ledValue = GPIO_GRP_0_PIN_0_PIN | GPIO_GRP_0_PIN_1_PIN;
            else if (movingAverageOutput > 40)
                ledValue = GPIO_GRP_0_PIN_0_PIN;

            DL_GPIO_writePinsVal(GPIO_GRP_0_PORT, ledMask, ledValue);

            DL_TimerA_startCounter(TIMER_0_INST);
            break;
        default:
            break;
    }
}