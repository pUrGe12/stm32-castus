#define REG(a) (*(volatile unsigned*)(a))
void uart_init(void){
  REG(0x40023830) |= 1u;          // RCC_AHB1ENR: GPIOA
  REG(0x40023840) |= (1u<<17);    // RCC_APB1ENR: USART2
  REG(0x40020000) &= ~(3u<<4);    // GPIOA_MODER PA2..
  REG(0x40020000) |=  (2u<<4);    // ..to AF
  REG(0x40020020) &= ~(0xFu<<8);  // AFRL PA2..
  REG(0x40020020) |=  (7u<<8);    // ..AF7
  REG(0x40004408)  = 0x0683;      // BRR (irrelevant in Renode)
  REG(0x4000440C)  = (1u<<13)|(1u<<3); // CR1: UE | TE
}
void uart_putc(char c){ while(!(REG(0x40004400)&(1u<<7))); REG(0x40004404)=c; } // wait TXE
void uart_write(const char* s){ while(*s) uart_putc(*s++); }