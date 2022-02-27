#define HOURS 3
#define MINUTES 15
#define SECONDS 55

#define HR_TEN    0xB0
#define HR_UNIT   0x70
#define MIN_TEN   0xD0
#define MIN_UNIT  0xE0

#define ZERO  0x7F
#define ONE   0x48
#define TWO   0xBA
#define THREE 0xEA
#define FOUR  0xCC
#define FIVE  0xE6
#define SIX   0xF6
#define SEVEN 0x4A
#define EIGHT 0xFF
#define NINE  0xEF

#define RTC_VCC PORTC.B5
#define RTC_GND PORTD.B0

char seven_seg_decoder[10] = {ZERO, ONE, TWO, THREE, FOUR, FIVE,
                               SIX, SEVEN, EIGHT, NINE};
char digits[4] = {HR_TEN, HR_UNIT, MIN_TEN, MIN_UNIT};
char display_latches[4] = {1,0,3,8};    // HH:MM
char current_digit = 0;

int ms = 0;
char min = 0;
char hrs = 0;
char sec = 0;

char int_flag; // Define variable cnt
char led_value = 0;

void interrupt() {
 int_flag = 1; // Interrupt causes cnt to be incremented by 1
 ms++;
 TMR0 = 194; // Timer TMR0 is returned its initial value
 INTCON = 0x20; // Bit T0IE is set, bit T0IF is cleared
}

void init_timer0_int(){
  OPTION_REG = 0x84; // Prescaler is assigned to timer TMR0
  TMR0 = 194;      // (256-63)+1. preset for timer register. 1mSec @8MHz xtal
  INTCON = 0xA0; // Enable interrupt TMR0
}
void update_time_latch(char h, char m){
    display_latches[0] = (int) h/10;
    display_latches[1] = (int) h%10;
    display_latches[2] = (int) m/10;
    display_latches[3] = (int) m%10;
}

unsigned short ds3231_read(unsigned short address){                 // ds3231 RTC module read
         unsigned short read_data;
         I2C1_Start();             // Start I2C signal
         I2C1_Wr(0xD0);                // Device address + W
         I2C1_Wr(address);              // Data address
         I2C1_Repeated_Start();
         I2C1_Wr(0xD1);                 // Device address + R
         read_data=I2C1_Rd(0);         // no acknowladge
         I2C1_Stop();                  // Stop I2C signal
         return(read_data);            // return read data
}

unsigned short ds3231_write(unsigned short address, unsigned short write_data){         // ds3231 RTC module write
         I2C1_Start();              // I2C start signal
         I2C1_Wr(0XD0);              // Device address +  W
         I2C1_Wr(address);            // Data adress
         I2C1_Wr(write_data);          // Data write
         I2C1_Stop();                  // stop I2C signal
}

void get_time(){
    sec = ds3231_read(0);
    min = ds3231_read(1);
    hrs = ds3231_read(2);

    sec = (sec >> 4) * 10 + (sec & 0x0F);
    min = (min >> 4) * 10 + (min & 0x0F);
    hrs = (hrs >> 4) * 10 + (hrs & 0x0F);
    
    update_time_latch(hrs,min);
}

void set_time(){
    unsigned int s,m,h;

    s = SECONDS;
    h = HOURS;
    m = MINUTES;

    s = ((s/10) << 4) | ((s%10) & 0x0F);
    m = ((m/10) << 4) | ((m%10) & 0x0F);
    h = ((h/10) << 4) | ((h%10) & 0x0F);
    ds3231_write(0, s);
    ds3231_write(1, m);
    ds3231_write(2, h);

}                    // common anode

void main() {
     init_timer0_int();
     TRISB = 0;           // set direction to be output
     TRISD &= 0x0F;
     TRISA.B0 = 0x00;
     
     PORTD |= 0xF0;
     
     I2C1_Init(100000);                          // Initialize I2C signal at 100KHz
     I2C1_Start();                               // Start I2C signal
     PORTA.B0 = 1;
     TRISD.B0 = 0x00;
     TRISC.B5 = 0x00;

     RTC_GND = 0x00;
     RTC_VCC = 0x01;
     set_time();
     get_time();
     while(1){
       if(int_flag){
        int_flag = 0;
        PORTD = (digits[current_digit] & 0xF0) |  (PORTD & 0x0F);
        PORTB = seven_seg_decoder[display_latches[current_digit]] ;
        current_digit = (current_digit+1)%4;
       }
       
       if(ms == 1000){
             ms = 0;
             sec++;
             led_value = led_value? 0x00: 0x01;
             PORTA.B0 = led_value;
             if(sec == 60){
             sec = 0;
             min++;
             if(min == 60){
                    min = 0;
                    hrs++;
                    if (hrs == 13){
                       hrs = 1;
                    }
             }
             update_time_latch(hrs,min);
          }
       }
  }

}