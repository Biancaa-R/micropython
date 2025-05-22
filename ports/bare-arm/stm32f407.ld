

MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 0x100000 /* entire flash, 1 MiB */
    //Should be till 0x080f ffff 
    //Matches it
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 0x020000 /* 128 KiB */
    //Should be till 0x2001 ffff
    //increase 0020 0000
    //Perfect
}
//Assigning the end address for stack
_estack = ORIGIN(RAM) + LENGTH(RAM); //More like top address pola as the stack is fd
//.text .rodata ---> Stored in flash memory --> I dont have to change it
//KEEP for reserving the data at begining of flash even when it is un used :)
//_etext -->to signify the end of the text section
//after ro data --> initialized data _sidata
SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
        _sidata = _etext;
    } >FLASH
    
    .data : AT ( _sidata )
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >RAM

    .heap :
    {
        . = ALIGN(8);
        _heap_start = .;
    } >RAM
}
