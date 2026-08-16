#include <stdint.h>
#include <string.h>

extern uint8_t _sbss, _ebss ,_skernel_bss, _ekernel_bss;
extern uint8_t _sidata, _sdata,_sikernel_data,_skernel_data;
extern uint32_t _sizedata, _sizekernel_data;

__attribute__((section(".boot_section"))) void init_bss(void){
    for( uint8_t* ptr = &_sbss; ptr< &_ebss ; ptr++){
        *ptr=0;
    }
    for( uint8_t* k_ptr = &_skernel_bss; k_ptr< &_ekernel_bss ; k_ptr++){
        *k_ptr=0;
    }

}
__attribute__((section(".boot_section"))) void load_to_ram(void){
    //Copy .data section
    memcpy(&_sdata, &_sidata, (size_t)&_sizedata);
    
    //Copy .kernel_data section
    memcpy(&_skernel_data, &_sikernel_data, (size_t)&_sizekernel_data);
}