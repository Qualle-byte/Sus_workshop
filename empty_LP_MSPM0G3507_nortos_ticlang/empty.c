#include "ti_msp_dl_config.h"

/*You can define your own global variables here, the three variables below are used as an example which could be useful for this task*/
volatile int32_t bufferSample[1000]={0};     //the ring buffer to store the (squared) value converted by ADC
volatile int32_t sampleIndex=0;     //show at which position in the ring buffer should the current value be stored
volatile int32_t movingAverageOutput = 0;      //the calculated moving average result

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
			
			/*end of your code*/
			DL_TimerA_startCounter(TIMER_0_INST);  //start the timer again for the next ADC conversion
            break;
        default:
            break;
    }
}

/*You can also define extra functions here below if needed*/
