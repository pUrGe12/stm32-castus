extern unsigned _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern void __libc_init_array(void);
extern int main(void);

void Default_Handler(void){ for(;;); }   // HardFault lands here -> Renode shows PC stuck

void Reset_Handler(void){
  *(volatile unsigned*)0xE000ED88 |= (0xFu<<20);   // CPACR: enable FPU (hard-float build!)
  unsigned *s=&_sidata,*d=&_sdata;
  while(d<&_edata) *d++=*s++;
  for(d=&_sbss; d<&_ebss;) *d++=0;
  __libc_init_array();
  main();
  for(;;);
}

__attribute__((section(".isr_vector"),used))
void (* const vtable[])(void) = {
  (void(*)(void))&_estack,   // 0: initial SP
  Reset_Handler,             // 1: reset
  Default_Handler,           // 2: NMI
  Default_Handler,           // 3: HardFault
};

void _init(void){}
void _fini(void){}