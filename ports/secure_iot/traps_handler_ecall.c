/***************************************************************************
 * Project           		: shakti devt board
 * Name of the file	     	: traps.c
 * Brief Description of file    : source file for first level of trap handling.
 * Name of Author    	        : Sathya Narayanan N
 * Email ID                     : sathya281@gmail.com

 Copyright (C) 2019  IIT Madras. All rights reserved.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ***************************************************************************/
/**
@file traps.c
@brief source file for first level of trap handling.
@details This file contains Trap handler routines.
*/

#include "traps.h"
#include "log.h"
#include "secure_iot.h"
#include "encoding.h"
#include "plic.h"
#include "mpstate.h"
#include  <stdbool.h>

extern sched_len;
extern plic_fptr_t isr_table[PLIC_MAX_INTERRUPT_SRC]; //isr_table maps interrupt IDs to function pointers.
extern interrupt_data_t hart0_interrupt_matrix[PLIC_MAX_INTERRUPT_SRC];
extern mp_state_ctx_t mp_state_ctx;
extern  mp_state_ctx_t;
#define MP_STATE_VM(x) (mp_state_ctx.vm.x)

#define PLIC_CLAIM_OFFSET               0x200004UL
mtrap_fptr_t mcause_trap_table[MAX_TRAP_VALUE];
mtrap_fptr_t mcause_interrupt_table[MAX_INTERRUPT_VALUE];
//#define __riscv_xlen 64
extern void *interrupt_arg[];
uint16_t interrupt_id;
static const char* mcause_messages[] = {
    "Instruction address misaligned",
    "Instruction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store/AMO address misaligned",
    "Store/AMO access fault",
    "Environment call from U-mode",
    "Environment call from S-mode",
    "Reserved trap 1",
    "Environment call from M-mode",
    "Instruction Page Fault",
    "Load Page Fault",
    "Reserved trap 2",
    "Store/AMO Page Fault"
};

void mcause_reason(MCAUSE_REASON mcause_number) {
    if (mcause_number < sizeof(mcause_messages) / sizeof(mcause_messages[0])) {
        log_error("%s\n", mcause_messages[mcause_number]);
    } else {
        log_error("Unknown mcause number: %d\n", mcause_number);
    }
}

/** @fn unsigned int extract_ie_code(unsigned int num)
 * @brief Extract the exception code from the mcause value
 * @details Extract the exception code from the mcause value
 *	    by masking the most significant bit and return
 *	    the exception code.
 * @param unsigned int num - mcause value
 * @return unsigned int
 */
unsigned int extract_ie_code(unsigned int num)
{
	unsigned int exception_code;

	exception_code = (num & 0X7FFFFFFF);

	log_error("Exception Code = %x\n",exception_code);

	return exception_code;
}

/** @fn void default_handler(uintptr_t mcause, uintptr_t epc)
 * @brief default handler that loops infinitely 
 * @param unsigned int ptr mcause
 * @param unsigned int ptr epc
 */
void default_handler(__attribute__((unused)) uintptr_t cause)
{
	while(1);
}

/** @fn uintptr_t Trap_Handler(uintptr_t mcause, uintptr_t epc)
 * @brief Handles the trap, exception or interrupt is determined here
 * @details Trap handler routine, which identifies the cause of trap 
 *	    and calls the respective trap handler.
 * @param unsigned int ptr mcause
 * @param unsigned int ptr epc
 * @return unsigned int ptr
 */
uintptr_t Trap_Handler(uintptr_t cause, uintptr_t epc)
{
	unsigned int ie_entry = 0;;
	uint32_t shift_length = 0;
	uintptr_t code=0;

	/*
	   risc v priv spec v1.10 section 3.1.20 Machine Cause Register (mcause)

	   XLEN -1   XLEN-2                                           0
	   ------------------------------------------------------------
	   | Interrupt |    Exception code                            |
	   ------------------------------------------------------------
	   1                            XLEN-1

	   The mcause register is an XLEN-bit read-write register formatted as shown in Figure 3.2
	   1. When a trap is taken into M-mode, mcause is written with a code indicating the event that caused the trap.
	   Otherwise, mcause is never written by the implementation, though it may be explicitly written by software.
	 */


	/*
	   The Interrupt bit in the mcause register is set if the trap was caused by an interrupt.
	   The Exception Code field contains a code identifying the last exception
	 */

	shift_length = __riscv_xlen - 1;
	//code = cause & (1 << (shift_length)); //To check if it ws cused by an error or an exception
	code = cause & (1ULL << shift_length); //changes made by Biancaa :)
	//So if it was 1 -> It considered as 32bit integer and the functionality didnt work.
	
	 /* checking for type of trap */
	if (code){
		//----------------------------------------------------------------------------------------------
    uint32_t mcause_code = cause & ~(1ULL << shift_length);

    if (mcause_code == 11) {
        uint32_t *claim_addr = (uint32_t *)(PLIC_BASE + PLIC_CLAIM_OFFSET);
        uint32_t interrupt_id = *claim_addr;

        if (isr_table[interrupt_id]) {
            isr_table[interrupt_id](interrupt_arg[interrupt_id]);
        }
		//mp_handle_pending();
        //mp_handle_pending(false);
		if (MP_STATE_VM(sched_len) > 0) {
    		// There is at least one pending scheduled handler
    		mp_handle_pending(false);  // false = don't raise exceptions
			*claim_addr = interrupt_id; // acknowledge
			hart0_interrupt_matrix[interrupt_id].state = SERVICED;
			hart0_interrupt_matrix[interrupt_id].count++;
		}
        //so the interrupt is handled on the fly without creating exceptions
        //just to ensure no interrupt is pending
		else{
        *claim_addr = interrupt_id; // acknowledge
		hart0_interrupt_matrix[interrupt_id].state = SERVICED;
		hart0_interrupt_matrix[interrupt_id].count++;
		}

    }
		//----------------------------------------------------------------------------------------------
	else{	
		ie_entry = (~code) & cause;

		log_debug("\nInterrupt: mcause = %x, epc = %x\n", cause, epc);
        uint32_t *interrupt_claim_address = (uint32_t *)(PLIC_BASE +
					       PLIC_CLAIM_OFFSET);
		interrupt_id= *interrupt_claim_address;
		//the interrupt id is stored in the address of the interrupt claim address.
		mcause_interrupt_table[ie_entry]((uint64_t) interrupt_arg[interrupt_id]);
	}
	}
  else if((cause && 0xFF) ==11){
    mcause_reason(cause);
    //Optionally fetch arguments from a0, a1, etc.
    // Optionally perform service or set return value in a0
		mcause_reason(cause); //Prnts the cause of the exception that has occured
		log_error("\nProblem at address 0x%x\n", epc);
    unsigned long mepc_val = read_csr(mepc);

    // Advance to the next instruction (skip current)
    unsigned long val = (mepc_val + 4) & ~(0x3UL);

    // Clear MSB (bit XLEN-1)
    val &= ~(1UL << (XLEN - 1));

    // Write back the updated mepc
    write_csr(mepc, val);
    unsigned long mstatus_val = read_csr(mstatus);
    // No need to change MPP if you want to return to M-mode
    // But make sure MPIE is set so interrupts restore correctly
    mstatus_val |= (1UL << 7); // Set MPIE bit
    write_csr(mstatus, mstatus_val);

    // Return from trap
    __asm__ volatile ("mret");
  }
	/*else if ((cause & 0xFFF) == 11) { // ecall from M-mode
		//mcause_reason(cause);
        // Optionally fetch arguments from a0, a1, etc.
        // Optionally perform service or set return value in a0
		mcause_reason(cause); //Prnts the cause of the exception that has occured
		log_error("\nProblem at address 0x%x\n", epc);
        // Advance mepc to skip 'ecall' instruction
		unsigned long mepc_val= read_csr(mepc);
		unsigned long val=mepc_val+4;
        //write_csr(mepc, val);
		//write_csr(mepc,0x9002dbfc);
		//write_csr(x1, val);
		__asm__ volatile ("mv x1, %0" : : "r"(val));
		//__asm__ volatile ("ret");
		//__asm__ volatile ("mret");
		//return;
		//all three corrupt the PC value

	}*/
	// else if ((cause & 0xFFF) == 1) { 

	// 	mcause_reason(cause); //Prnts the cause of the exception that has occured
	// 	log_error("\nProblem at address 0x%x\n", epc);
	// 	unsigned long mepc_val= read_csr(mepc);
	// 	unsigned long val=mepc_val+2;
    //     //write_csr(mepc, val);
	// 	//write_csr(x1, val);
	// 	__asm__ volatile ("mv x1, %0" : : "r"(val));
	// 	__asm__ volatile ("li a0, 0");
	// 	__asm__ volatile ("ret");
	// 	//__asm__ volatile ("mret");

	// }
	else{
		mcause_reason(cause); //Prnts the cause of the exception that has occured
		log_error("\nProblem at address 0x%x\n", epc);

		// mcause_trap_table[mcause](mcause, epc);
		write_csr(mcause, 0);
		write_csr(mepc, 0); //To prevent any execution
	}

	return epc;
	//puts the value into a0 , a0->mepc
}
