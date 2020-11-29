//27.11.2020
// fuses LOW 39 HIGH FB
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
volatile uint32_t milis = 0;
volatile boolean milis_changed = 0;
volatile boolean pulse_catched = 0;
volatile uint32_t milis_front = 0;
volatile uint32_t milis_back = 0;
uint32_t milis_bufered = 0;
uint32_t stop_time_milis = 0;
uint32_t detecting_timeout_milis = 0;
uint32_t led_milis = 0;
uint16_t positive_pulse = 0;
uint16_t negative_pulse = 0;
boolean front_catched = 0;
int8_t detecting = 0;
boolean allow_enable_compressor = 0;
void setup() 
{
//DDRB = 0b00000100; // set PB2 pin as OUTPUT
//pinMode ( PB0, OUTPUT);
//pinMode ( PB1, INPUT);
//pinMode ( PB2, OUTPUT);
//pinMode ( PB3, OUTPUT);
//pinMode ( PB4, OUTPUT);
DDRB |= (1 << PB0) | (1 << PB2) | (1 << PB3) | (1 << PB4);
DDRB &= ~(1 << PB1);
//digitalWrite ( PB0, LOW);
PORTB &= ~(1 << PB0);
//digitalWrite ( PB2, HIGH);
PORTB |= (1 << PB2);
//digitalWrite ( PB3, LOW);
PORTB &= ~(1 << PB3);
//digitalWrite ( PB4, LOW);
PORTB &= ~(1 << PB4);
// Прерывание INT0
//MCUCR = (0 << ISC01) | (1 << ISC00);
MCUCR |= (1 << ISC00);
GIMSK |= (1 << INT0);
// Таймер 
TCCR0A |= (1<<WGM01); // режим подсчета импульсов (сброс при совпадении)
//  TCCR0B = (1 << CS02)|(0 << CS01)|(0 << CS00); // предделитель
TCCR0B |= (1 << CS02); // предделитель
TCNT0 = 0x00; // начальное значение счётчика импульсов
OCR0A = 18; // максимальный предел счета (0-255)
TIMSK0 |= (1 << OCIE0A); // разрешение прерывания по совпадению со значением регистра OCR0A
wdt_enable(WDTO_8S);
wdt_reset();
//WDTO_15MS     // 15 мс
//WDTO_30MS     // 30 мс
//WDTO_60MS     // 60 мс
//WDTO_120MS    // 120 мс
//WDTO_250MS    // 250 мс
//WDTO_500MS    // 500 мс
//WDTO_1S       // 1 сек
//WDTO_2S       // 2 сек
//WDTO_4S       // 4 сек
//WDTO_8S       // 8 сек
asm("sei");
}

void loop() {
if (milis_changed)
  {
    wdt_reset();
    milis_changed = 0;
    asm("cli");
    milis_bufered = milis;
    asm("sei");
    if ((milis_bufered - detecting_timeout_milis) > 6000)
      {
        detecting = detecting - 1;
        detecting_limit();
      }
    if (allow_enable_compressor)
      {
        if (detecting < 4)
          {
            allow_enable_compressor = 0;  
            stop_time_milis = milis_bufered;
          }
      }
    else
      {
        if ((detecting > 4) && ((milis_bufered - stop_time_milis) > 250000))
        {
           allow_enable_compressor = 1;
        }
      }
    digitalWrite (PB0, allow_enable_compressor);
    if ((milis_bufered - led_milis) > 300)
      {
//        digitalWrite (PB3, LOW);
		PORTB &= ~(1 << PB3);
//        digitalWrite (PB4, LOW);
		PORTB &= ~(1 << PB4);
      }
  }
if (pulse_catched)
  {
    pulse_catched = 0;
    asm("cli");
    if (milis_front > milis_back)
      {
        negative_pulse = milis_front - milis_back;
    front_catched = 1;
      }
    else
      {
        positive_pulse = milis_back - milis_front;
    front_catched = 0;
      }
    asm("sei");
  if ((positive_pulse > 500) && (!front_catched)) {negative_pulse =  0;}
    if ((positive_pulse > 1600) && (positive_pulse < 1900) && front_catched)
      {
        detecting_timeout_milis = milis_bufered;
        if ((negative_pulse > 6) && (negative_pulse < 12))
          {
            detecting++;
            detecting_limit();
//            digitalWrite (PB3, HIGH);
			PORTB |= (1 << PB3);
            led_milis = milis_bufered;
          }
        else
        {
            detecting--;
            detecting_limit();  
//            digitalWrite (PB4, HIGH);  
			PORTB |= (1 << PB4);
            led_milis = milis_bufered;     
        }
      }
  }
}

ISR(TIM0_COMPA_vect)
{
milis++;
milis_changed = 1;
// PORTB ^= _BV(PB2);
}

//Обработчик прерывания INT0
ISR(INT0_vect)
{
pulse_catched = 1;
if(PINB & (1 << PB1))
{
  PORTB &= ~(1 << PB2);
  milis_front = milis;
}
else
{
  milis_back = milis;
  PORTB |= (1 << PB2);
}
}

void detecting_limit()
{
  if (detecting > 6)
  {
    detecting = 6;
  }
  else if (detecting < 0)
  {
    detecting = 0;
  }
}
