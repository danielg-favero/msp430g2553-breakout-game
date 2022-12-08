#include <msp430.h>

void start_P1_P2(void);
void start_micro(void);
void start_TA0_ADC10_Joystick(void);
void start_ADC10_Joystick(void);

unsigned int ADC10_vector[4];
unsigned int avarage;
unsigned int sum;
unsigned char i;
unsigned int count = 0;

int main(void)
{
    start_P1_P2();
    start_micro();
    start_ADC10_Joystick();
    start_TA0_ADC10_Joystick();

    do {

    } while(1);
}


void start_micro(void){
    WDTCTL = WDTPW | WDTHOLD;

    // Configurações do BCS
    // MCLK = DCOCLK ~ 16MHz
    // ACLK = LFXT1CLK = 32768Hz
    // SMCLK = DCOCLK / 8 ~ 2MHz

    DCOCTL = CALDCO_16MHZ;
    BCSCTL1 = CALBC1_16MHZ;
    BCSCTL2 = DIVS1;
    BCSCTL3 = XCAP0 + XCAP1;

    while(BCSCTL3 & LFXT1OF);

    __enable_interrupt();

}

void start_P1_P2(void){
    /*
        PORTA 1:
        P1.0 - Entrada Analógica - Eixo X
        P1.X - N.C.              - Saída nível baixo
    */
    P1DIR = 0xFF;
    P1OUT = 0;

    /*
        PORTA 2:
        P2.X - N.C. - Saída nível baixo
     */
    P2DIR = 0xFF;
    P2IFG = 0;
}

void start_ADC10_Joystick(void){
    /*
        Taxa de amostragem: 200ksps
        Vref:               Vcc
        Tsample:
            - Ts:           (6k + 2k) * 27p * ln(2^11) ~ 2,05us
            - nº ciclos:    2,05us * 6.3Mhz = 12,96 ~ 16
        Buffer:             Sem Buffer
    */
    ADC10CTL0 = ADC10SHT1 + MSC + ADC10ON + ADC10IE;
    ADC10CTL1 = SHS0 + CONSEQ0 + CONSEQ1 + INCH0;

    ADC10AE0 = BIT0;

    ADC10DTC0 = 0;
    ADC10DTC1 = 4;

    ADC10SA = &ADC10_vector[0];

    ADC10CTL0 |= ENC;
}

void start_TA0_ADC10_Joystick(void){
    /*
        Frequência de amostragem (fa) = 5Khz
        Tempo de amostragem (ta) = 1 / fa = 200us

        CONTADOR:
        Clock:              SMCLK ~ 2Mhz
        Fator de divisão:   1
        Modo:               UP
        Interrupção:        Desabilitada

        MÓDULO 0:
        Função:             Comparação
        TA0CCR0:            (200u * 2M / 1 - 1) = 399
        Interrupção:        Desabilitada

        MÓDULO 1:
        Função:             Comparação
        TA0CCR1:            199 (50% de razão cíclica)
        Interrupção:        Desabilitada
     */

    TA0CTL = TASSEL1 + MC0;
    TA0CCTL1 = OUTMOD2 + OUTMOD1 + OUTMOD0;
    TA0CCR0 = 399;
    TA0CCR1 = 199;
}

// ---------- INTERRUPÇÕES ----------

#pragma vector=ADC10_VECTOR
__interrupt void RTI_ADC10(){
    ADC10CTL0 &= ~ENC;

    sum = 0;

    for(i = 1; i < 4; i += 2){
        sum += ADC10_vector[i];
    }
    avarage = sum >> 2; // divisão por 8

    if(avarage > 1){
        if(avarage > 270){
            // Direita
        } else if(avarage < 240){
            // Esquerda
        }
    }


    ADC10SA = &ADC10_vector[0];
    ADC10CTL0 |= ENC;
}
