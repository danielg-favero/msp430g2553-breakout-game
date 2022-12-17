#include "msp430.h"
#include <stdint.h>

// Definicao das funcoes da porta 1
#define JOYSTICK    BIT0
#define SWITCH      BIT5
#define SPI_SIMO    BIT2
#define SPI_LOAD    BIT3
#define SPI_CLK     BIT4

// Constantes do jogo
#define PLAYER_MAX_RIGHT 0b00000111
#define PLAYER_MAX_LEFT 0b11100000
#define LEFT 0
#define RIGHT 1
#define DOWN 0
#define UP 1
#define TRUE 1
#define FALSE 0

// Registradores do MAX7219
#define NOOP    0x00
#define DIGIT0  0x01
#define DIGIT1  0x02
#define DIGIT2  0x03
#define DIGIT3  0x04
#define DIGIT4  0x05
#define DIGIT5  0x06
#define DIGIT6  0x07
#define DIGIT7  0x08
#define DECODEMODE  0x09
#define INTENSITY   0x0A
#define SCANLIMIT   0x0B
#define SHUTDOWN    0x0C
#define DISPLAYTEST 0x0F

unsigned char disp = 0;
unsigned int ADC10_vector[8];
unsigned int avarage;
unsigned int sum;
unsigned char i;
unsigned int count = 0;
int directionX = LEFT;
unsigned int directionY = DOWN;
unsigned int game_over = FALSE;
unsigned int ball_index = 5;
unsigned int is_centered = TRUE;

// Display nos LED
uint8_t MAP[16] = {
         0b11111111,
         0b11111111,
         0b11111111,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00111000
 };

uint8_t MAP_BACKUP[16] = {
         0b11111111,
         0b11111111,
         0b11111111,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00111000
 };


uint8_t BALL_MAP[16] = {
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00100000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000
};

uint8_t BALL_MAP_BACKUP[16] = {
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00100000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000,
         0b00000000
};

void start_micro(void);
void start_p1_p2(void);
void start_usci_spi(void);
void start_TA0_ADC10_Joystick(void);
void start_ADC10_Joystick(void);
void start_TA1_Debouncer(void);
void clear_matrix(void);
void print_matrix(void);
void update_matrix(uint8_t address, uint8_t data);
void send_data(uint8_t address, uint8_t data);
void reset_game(void);

int main(void) {
    __enable_interrupt();

    start_p1_p2();
    start_micro();
    start_ADC10_Joystick();
    start_TA0_ADC10_Joystick();
    start_TA1_Debouncer();
    start_usci_spi();

    send_data(NOOP, 0x00);
    send_data(SCANLIMIT, 0x07);
    send_data(INTENSITY, 0x01);
    send_data(DECODEMODE, 0);

    clear_matrix();

    send_data(SHUTDOWN, 1);

    do {
        print_matrix();
    } while(1);

}

void start_micro(){
    WDTCTL = WDTPW + WDTHOLD;

    DCOCTL = CALDCO_16MHZ;
    BCSCTL1 = CALBC1_16MHZ;
    BCSCTL2 = DIVS0 + DIVS1;
    BCSCTL3 = XCAP0 + XCAP1;

    while(BCSCTL3 & LFXT1OF);
}

void start_p1_p2(){
    /*
        PORTA 1:
        P1.2 - SPI          - Saida em nivel baixo
        P1.3 - Chip Select  - Saida em nivel baixo
        P1.4 - Clock        - Saida em nivel baixo
        P1.x - N.A          - Saida em nivel baixo
    */
    P1DIR |= SPI_SIMO + SPI_CLK + SPI_LOAD;
    P1REN = SWITCH;
    P1OUT = SWITCH;
    P1IES = SWITCH;
    P1IFG = 0;
    P1IE = SWITCH;

    /*
        PORTA 2:
        P2.x - N.A          - Saida em nivel baixo
    */
    P2DIR = 0xFF;
    P2OUT = 0;
}

void start_usci_spi() {
    UCA0CTL1 |= UCSWRST;

    UCA0CTL0 = UCCKPH + UCMSB + UCMST + UCSYNC;
    UCA0CTL1 |= UCSSEL1;

    UCA0BR0 |= 0x01;
    UCA0BR1 = 0;

    P1SEL |= SPI_SIMO + SPI_CLK;
    P1SEL2 |= SPI_SIMO + SPI_CLK;

    UCA0CTL1 &= ~UCSWRST;
}

void start_ADC10_Joystick(void){
    /*
        Taxa de amostragem: 200ksps
        Vref:               Vcc
        Tsample:
            - Ts:           (10k + 2k) * 27p * ln(2^11) ~ 2,47us
            - nº ciclos:    2,47us * 6.3Mhz = 15,56 ~ 16
        Buffer:             Sem Buffer
    */
    ADC10CTL0 = ADC10SHT1 + MSC + ADC10ON + ADC10IE;
    ADC10CTL1 = SHS0 + CONSEQ0 + CONSEQ1 + INCH0;

    ADC10AE0 = JOYSTICK;

    ADC10DTC0 = 0;
    ADC10DTC1 = 4;

    ADC10SA = &ADC10_vector[0];

    ADC10CTL0 |= ENC;
}

void start_TA0_ADC10_Joystick(void){
    /*
        Tempo de amostragem (ta) = 1 / fa = 250m

        CONTADOR:
        Clock:              SMCLK ~ 2Mhz
        Fator de divisão:   8
        Modo:               UP
        Interrupção:        Desabilitada

        MÓDULO 0:
        Função:             Comparação
        TA0CCR0:            (250m * 2M / 8 - 1) = 62499
        Interrupção:        Habilitada

        MÓDULO 1:
        Função:             Comparação
        TA0CCR1:            31249
        Interrupção:        Desabilitada
    */
    TA0CTL = TASSEL1 + MC0 + ID0 + ID1;
    TA0CCTL0 = CCIE;
    TA0CCTL1 = OUTMOD2 + OUTMOD1 + OUTMOD0;
    TA0CCR0 = 62499;
    TA0CCR1 = 31249;
}

void start_TA1_Debouncer(void){
    /*
        Tempo de debounce: 10ms

        CONTADOR:
        Clock:            ACLK
        Fator de divisão: 1
        Modo:             UP
        Interrupção:      Desabilitado

        MÓDULO 0:
        Função:           Comparação
        TA0CCR0:          (10m * 32768 / 1 - 1) = 326
        Interrupção:      Habilitada
    */
    TA1CTL = TASSEL0;
    TA1CCTL0 = CCIE;
    TA1CCR0 = 326;
}

void clear_matrix() {
    send_data(DIGIT0, 0);
    send_data(DIGIT1, 0);
    send_data(DIGIT2, 0);
    send_data(DIGIT3, 0);
    send_data(DIGIT4, 0);
    send_data(DIGIT5, 0);
    send_data(DIGIT6, 0);
    send_data(DIGIT7, 0);
}

void update_matrix(uint8_t address, uint8_t data) {
    if(disp == 0){
        P1OUT |= SPI_LOAD;
    } else{
        disp = 0;
    }

    UCA0TXBUF = address & 0b00001111;
    while (UCA0STAT & UCBUSY);

    UCA0TXBUF = data;
    while (UCA0STAT & UCBUSY);

    P1OUT &= ~(SPI_LOAD);
}

void send_data(uint8_t address, uint8_t data) {
    /*
        FORMATO DO DADO A SER ENVIADO
        15  14  13  12  11  10  9   8   7   6   5   4   3   2   1
        x   x   x   x   |  ADDRESS  |  MSB          DATA       LSB
    */

    UCA0TXBUF = address & 0b00001111;
    while (UCA0STAT & UCBUSY);

    UCA0TXBUF = data;
    while (UCA0STAT & UCBUSY);

    P1OUT |= SPI_LOAD;
    P1OUT &= ~(SPI_LOAD);
}

void print_matrix(){
    int row;

    for(row = 0; row < 8; row++) {
        disp = 1;
        uint8_t current_row = BALL_MAP[row] | MAP[row];
        update_matrix(DIGIT0 + row, current_row);
        current_row = BALL_MAP[row + 8] | MAP[row + 8];
        update_matrix(DIGIT0 + row, current_row);
    }
}

void reset_game(){
    int row;

    game_over = FALSE;
    TA0CTL |= MC0;
    TA0CCR0 = 62499;
    ball_index = 5;

    for(row = 0; row < 16; row++) {
        MAP[row] = MAP_BACKUP[row];
        BALL_MAP[row] = BALL_MAP_BACKUP[row];
    }
}


// ---------- INTERRUPÇÕES ----------

#pragma vector=ADC10_VECTOR
__interrupt void RTI_ADC10(){
    ADC10CTL0 &= ~ENC;

    sum = 0;

    for(i = 1; i < 8; i++){
        sum += ADC10_vector[i];
    }
    avarage = sum >> 3;

    if(avarage > 1){
        if(avarage < 240){
            
            if(MAP[15] > PLAYER_MAX_RIGHT){
                MAP[15] = MAP[15] >> 1;
            }
        } else if(avarage > 270){
            
            if(MAP[15] < PLAYER_MAX_LEFT){
                MAP[15] = MAP[15] << 1;
            }
        }
    }

    ADC10SA = &ADC10_vector[0];
    ADC10CTL0 |= ENC;
}

#pragma vector = PORT1_VECTOR
__interrupt void PORT1_RTI(void) {
    P1IE &= ~SWITCH;
    TA1CTL |= MC0;
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void MODULE0_TIMER1_RTI(void) {
    TA1CTL &= ~MC0;

    if(~P1IN & SWITCH){
        reset_game();
    }

    P1IFG &= ~SWITCH;
    P1IE |= SWITCH;
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void MODULE0_TIMER0_RTI(void) {

    TA0CTL &= ~MC0;

    if(ball_index == 0) {
        directionY = !directionY;
    } else if (ball_index == 14 && (MAP[15] & BALL_MAP[14]) > 0) {
        directionY = !directionY;

        if(((MAP[15] << 1) & (MAP[15] >> 1)) == BALL_MAP[14]){
            is_centered = TRUE;
        } else {
            is_centered = FALSE;
        }
    } else if (ball_index == 15) {
        game_over = TRUE;
    }

    if(!game_over){
        if ((BALL_MAP[ball_index] << 1) > 128 || (BALL_MAP[ball_index] >> 1) == 0) {
           directionX = !directionX;
        }

        uint8_t temp_positionX =  directionX == LEFT ? BALL_MAP[ball_index] << !is_centered: BALL_MAP[ball_index] >> !is_centered ;

        unsigned int next_index = (directionY == UP) ? ball_index - 1 : ball_index + 1;
        if((MAP[next_index] & temp_positionX) > 0) {
            if(next_index == 15){
                directionY = !directionY;
                directionX = !directionX;

                if ((BALL_MAP[ball_index] << 1) > 128 || (BALL_MAP[ball_index] >> 1) == 0) {
                    directionX = !directionX;
                }

                temp_positionX = directionX == LEFT ? BALL_MAP[ball_index] << !is_centered: BALL_MAP[ball_index] >> !is_centered ;
            } else {
                MAP[next_index] &= ~temp_positionX;
                TA0CCR0 -= 1822;
                directionY = !directionY;
            }

            next_index = (directionY == UP) ? ball_index - 1 : ball_index + 1;
        }
        else if((MAP[next_index] & BALL_MAP[ball_index])>0){
            if (next_index != 15){
                MAP[next_index] &= ~BALL_MAP[ball_index];
                directionY = !directionY;
                next_index = (directionY == UP) ? ball_index - 1 : ball_index + 1;
            }
        }

        BALL_MAP[ball_index] = temp_positionX ;
        uint8_t temp_positionY = BALL_MAP[ball_index];
        BALL_MAP[ball_index] = BALL_MAP[next_index];
        BALL_MAP[next_index] = temp_positionY;

        ball_index = next_index;

        TA0CTL |= MC0;
    }

}



