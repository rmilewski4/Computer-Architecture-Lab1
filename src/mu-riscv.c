#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-riscv.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-RISCV Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Turn a byte to a word                                                                          */
/***************************************************************/
uint32_t byte_to_word(uint8_t byte)
{
    return (byte & 0x80) ? (byte | 0xffffff80) : byte;
}

/***************************************************************/
/* Turn a halfword to a word                                                                          */
/***************************************************************/
uint32_t half_to_word(uint16_t half)
{
    return (half & 0x8000) ? (half | 0xffff8000) : half;
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate RISCV for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/**************************************************************rdump*/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < RISCV_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-RISCV SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-RISCV! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < RISCV_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;
	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

void R_Processing(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t rs2, uint32_t f7) {
	switch(f3){
		case 0:
			switch(f7){
				case 0:		//add
					NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] + NEXT_STATE.REGS[rs2];
					break;
				case 32:	//sub
					NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] - NEXT_STATE.REGS[rs2];
					break;
				default:
					RUN_FLAG = FALSE;
					break;
				}	
			break;
		case 6: 			//or
			NEXT_STATE.REGS[rd] = (NEXT_STATE.REGS[rs1] | NEXT_STATE.REGS[rs2]);
			break;
		case 7:				//and
			NEXT_STATE.REGS[rd] = (NEXT_STATE.REGS[rs1] & NEXT_STATE.REGS[rs2]);
			break;
		default:
			RUN_FLAG = FALSE;
			break;
	} 			
}

void ILoad_Processing(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t imm) {
	switch (f3)
	{
	case 0: //lb
		NEXT_STATE.REGS[rd] = byte_to_word((mem_read_32(NEXT_STATE.REGS[rs1] + imm)) & 0xFF);
		break;

	case 1: //lh
		NEXT_STATE.REGS[rd] = half_to_word((mem_read_32(NEXT_STATE.REGS[rs1] + imm)) & 0xFFFF);
		break;

	case 2: //lw
		NEXT_STATE.REGS[rd] = mem_read_32(NEXT_STATE.REGS[rs1] + imm);
		break;
	
	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}
}

void Iimm_Processing(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t imm) {
	uint32_t imm0_4 = (imm << 7) >> 7;
	uint32_t imm5_11 = imm >> 5;
	switch (f3)
	{
	case 0: //addi
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] + imm;
		break;

	case 4: //xori
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] ^ imm;
		break;
	
	case 6: //ori
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] | imm;
		break;
	
	case 7: //andi
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] & imm;
		break;
	
	case 1: //slli
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] << imm0_4;
		break;
	
	case 5: //srli and srai
		switch (imm5_11)
		{
		case 0: //srli
			NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] >> imm0_4;
			break;

		case 32: //srai
			//NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] >> imm0_4;
			break;
		
		default:
			RUN_FLAG = FALSE;
			break;
		}
		break;
	
	case 2:
		break;

	case 3:
		break;

	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}
}

void S_Processing(uint32_t imm4, uint32_t f3, uint32_t rs1, uint32_t rs2, uint32_t imm11) {
	// Recombine immediate
	uint32_t imm = (imm11 << 5) + imm4;

	switch (f3)
	{
	case 0: //sb
		mem_write_32((NEXT_STATE.REGS[rs1] + imm), NEXT_STATE.REGS[rs2]);
		break;
	
	case 1: //sh
		mem_write_32((NEXT_STATE.REGS[rs1] + imm), NEXT_STATE.REGS[rs2]);
		break;

	case 2: //sw
		mem_write_32((NEXT_STATE.REGS[rs1] + imm), NEXT_STATE.REGS[rs2]);
		break;

	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}
}
void ECall_Processing() {
	//Need to read register 17 to discover operation
	uint32_t a2 = CURRENT_STATE.REGS[17];
	switch(a2) {
		//exit
		case 10:
			RUN_FLAG = FALSE;
			break;

		default:
			printf("Invalid ECall!");
			break;
	}
}
void B_Processing(uint32_t funct3, uint32_t rs1, uint32_t rs2, int32_t imm, int* branchTaken) {
	//int32_t imm = (imm12and10_5 & 64) << 5 | (imm4_1and11 & 1) << 10 | (imm12and10_5 & 63) << 4 | (imm4_1and11 & 30) >> 1;
	
	switch(funct3) {
		case 0: //beq
			if(CURRENT_STATE.REGS[rs1] == CURRENT_STATE.REGS[rs2]){
				NEXT_STATE.PC += imm;
				*branchTaken = 1;
			}
		break;
		case 1: //bne
			if(CURRENT_STATE.REGS[rs1] != CURRENT_STATE.REGS[rs2]){
				NEXT_STATE.PC += imm;
				*branchTaken = 1;
			}
		break;
		case 4: //blt
			if(CURRENT_STATE.REGS[rs1] < CURRENT_STATE.REGS[rs2]){
				NEXT_STATE.PC += imm;
				*branchTaken = 1;

			}
		break;
		case 5: //bgt
			if(CURRENT_STATE.REGS[rs1] >= CURRENT_STATE.REGS[rs2]){
				NEXT_STATE.PC += imm;
				*branchTaken = 1;
			}
		break;
		case 6: //bltu
			if(CURRENT_STATE.REGS[rs1] < CURRENT_STATE.REGS[rs2]){
				NEXT_STATE.PC += imm;
				*branchTaken = 1;

			}
		break;
		case 7: //bgtu
			if(CURRENT_STATE.REGS[rs1] >= CURRENT_STATE.REGS[rs2]){
				NEXT_STATE.PC += imm;
				*branchTaken = 1;
			}
		break;
		default:
			printf("Invalid instruction");
			RUN_FLAG = FALSE;
		break;
	}
	
}

void J_Processing() {
	// hi
}

void U_Processing() {
	// hi
}

/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/

void handle_instruction()
{
	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
	//Get instruction by reading current PC
	uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
		uint32_t rd = 0;
		uint32_t funct3 = 0;
		uint32_t rs1 = 0;
		uint32_t rs2 = 0;
		uint32_t funct7 = 0;
		uint32_t imm = 0;
		uint32_t imm2 = 0;
		int branchTaken = 0;
	//127 in base-10 is = 1111111 in base 2, which will allow us to extract the opcode from the instruction
	uint32_t opcode = instruction & 127;
	switch(opcode) {
		//R-type instructions
		case(51):
			rd = (instruction & 3968) >> 7;
			funct3 = (instruction & 28672) >> 12;
			rs1 = (instruction & 1015808) >> 15;
			rs2 = (instruction & 32505856) >> 20;
			funct7 = (instruction & 4261412864) >> 25;
			R_Processing(rd,funct3,rs1,rs2,funct7);
			break;
		//I-type Instructions
		case(19):
			rd = (instruction & 3968) >> 7;
			funct3 = (instruction & 28672) >> 12;
			rs1 = (instruction & 1015808) >> 15;
			imm = (instruction & 4293918720) >> 20;
			Iimm_Processing(rd,funct3,rs1,imm);
			break;
		//I-type load instructions
		case(3):
			rd = (instruction & 3968) >> 7;
			funct3 = (instruction & 28672) >> 12;
			rs1 = (instruction & 1015808) >> 15;
			imm = (instruction & 4293918720) >> 20;
			ILoad_Processing(rd,funct3,rs1,imm);
			break;
		//S-type instructions
		case(35):
			imm2 = (instruction & 3968) >> 7;
			funct3 = (instruction & 28672) >> 12;
			rs1 = (instruction & 1015808) >> 15;
			rs2 = (instruction & 32505856) >> 20;
			imm = (instruction & 4261412864) >> 25;
			S_Processing(imm2,funct3,rs1,rs2,imm);
			break;
		//B-type instructions
		case(99):{
		uint32_t funct3 = (instruction & 28672) >> 12;
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t rs2 = (instruction & 32505856) >> 20;
		int32_t imm11 = (instruction & 128) >> 7;
		int32_t imm4_1 = (instruction & 3840) >> 7;
		int32_t imm10_5 = (instruction & 2113929216) >> 25;
		int32_t imm12 = (instruction & 2147483648) >> 31;
		int32_t imm = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1); //13 bits long
		if(imm12 == 1){
			imm = 0xFFFFE000 | imm;
		}
			B_Processing(funct3, rs1, rs2, imm, &branchTaken);
			break;
		}
		//SYSCALL/ECall opcode
		case(115):
			ECall_Processing();
			break;
		default:
			printf("OPCODE NOT FOUND!\n\n");
			break;
	}
	if(branchTaken == 0) {
		//Updates program counter, each instruction is 4 bytes.
		NEXT_STATE.PC += 4;
	}
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	CURRENT_STATE.REGS[2] = MEM_STACK_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in RISCV assembly format)    */ 
/************************************************************/
void print_program(){
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE.PC = MEM_TEXT_BEGIN + 4;
	
	uint32_t addressMemory;
	int i = 0;
	while(RUN_FLAG == TRUE){
		addressMemory = CURRENT_STATE.PC;
		
		print_instruction(addressMemory);
		
		NEXT_STATE.PC += 4;
		i++;
	}
}

void R_Print(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t rs2, uint32_t f7) {
	switch(f3){
		case 0:
			switch(f7){
				case 0:		//add
					printf("add x%d, x%d, x%d\n\n", rd, rs1, rs2);
					break;
				case 32:	//sub
					printf("sub x%d, x%d, x%d\n\n", rd, rs1, rs2);
					break;
				default:
					RUN_FLAG = FALSE;
					break;
				}	
			break;
		case 6: 			//or
			printf("or x%d, x%d, x%d\n\n", rd, rs1, rs2);
			break;
		case 7:				//and
			printf("and x%d, x%d, x%d\n\n", rd, rs1, rs2);
			break;
		default:
			RUN_FLAG = FALSE;
			break;
	}

}

void S_Print(uint32_t imm11, uint32_t f3, uint32_t rs1, uint32_t rs2, uint32_t imm4){
	uint32_t imm = (imm4<< 5) + imm11;

	switch (f3)
		{
		case 0: //sb
			printf("sb x%d, %d(x%d)\n\n", rs2, imm, rs1);
			break;
		
		case 1: //sh		
			printf("sh x%d, %d(x%d)\n\n", rs2, imm, rs1);
			break;

		case 2: //sw
			printf("sw x%d, %d(x%d)\n\n", rs2, imm, rs1);
			break;

		default:
			RUN_FLAG = FALSE;
			break;
	}
	

}

void I_Print(uint32_t imm, uint32_t f3, uint32_t rs1, uint32_t rd){
	switch (f3)
	{
	case 0: //lb
		printf("lb x%d, %d(x%d)\n\n", rd, imm, rs1);
		break;

	case 1: //lh
		printf("lh x%d, %d(x%d)\n\n", rd, imm, rs1);
		
		break;

	case 2: //lw
		printf("lw x%d, %d(x%d)\n\n", rd, imm, rs1);
		break;
	
	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}
}

void Iimm_Print(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t imm){
	uint32_t imm0_4 = (imm << 7) >> 7;
	uint32_t imm5_11 = imm >> 5;
	switch (f3)
	{
	case 0: //addi
		printf("addi x%d, x%d, %d\n\n", rd, rs1, imm);
		break;

	case 4: //xori
		printf("xori x%d, x%d, %d\n\n", rd, rs1, imm);
		break;
	
	case 6: //ori
		printf("ori x%d, x%d, %d\n\n", rd, rs1, imm);
		break;
	
	case 7: //andi
		printf("andi x%d, x%d, %d\n\n", rd, rs1, imm);
		break;
	
	case 1: //slli
		printf("slli x%d, x%d, %d\n\n", rd, rs1, imm0_4); 
		break;
	
	case 5: //srli and srai
		switch (imm5_11)
		{
		case 0: //srli
			printf("srli x%d, x%d, %d\n\n", rd, rs1, imm0_4); 
			break;

		case 32: //srai
			printf("srai x%d, x%d, %d\n\n", rd, rs1, imm0_4); 
			break;
		
		default:
			RUN_FLAG = FALSE;
			break;
		}
		break;
	
	case 2:
		break;

	case 3:
		break;

	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}

}

void B_Print(int32_t imm, uint32_t funct3, uint32_t rs1, uint32_t rs2){
	switch(funct3) {
		case 0: //beq
			printf("beq x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 1: //bne
			printf("bne x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 4: //blt
			printf("blt x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 5: //bgt
			printf("bgt x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 6: //bltu
			printf("bltu x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;

		case 7: //bgtu
			printf("bgtu x%d, x%d, %d\n\n", rs1, rs2, imm);
			break;
		
		default:
			printf("Invalid instruction");
			RUN_FLAG = FALSE;
			break;
	}

}

/************************************************************/
/* Print the instruction at given memory address (in RISCV assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){

	uint32_t instruction = mem_read_32(addr);
	uint32_t maskopcode = 0x7F;
	uint32_t opcode = instruction & maskopcode;
	if(opcode == 51) { //R-type
		uint32_t maskrd = 0xF80;
		uint32_t rd = instruction & maskrd;
		rd = rd >> 7;
		uint32_t maskf3 = 0x7000;
		uint32_t f3 = instruction & maskf3;
		f3 = f3 >> 12;
		uint32_t maskrs1 = 0xF8000;
		uint32_t rs1 = instruction & maskrs1;
		rs1 = rs1 >> 15;
		uint32_t maskrs2 = 0x1F00000;
		uint32_t rs2 = instruction & maskrs2;
		rs2 = rs2 >> 20;
		uint32_t maskf7 = 0xFE000000;
		uint32_t f7 = instruction & maskf7;
		f7 = f7 >> 25;
		R_Print(rd,f3,rs1,rs2,f7);
	} else if(opcode == 3) { // I-type load
		uint32_t rd = (instruction & 3968) >> 7;
		uint32_t funct3 = (instruction & 28672) >> 12;
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t imm = (instruction & 4293918720) >> 20;
		I_Print(imm, funct3, rs1, rd);
	} else if(opcode == 19){ //I-type 
		uint32_t rd = (instruction & 3968) >> 7;
		uint32_t funct3 = (instruction & 28672) >> 12;
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t imm = (instruction & 4293918720) >> 20;
		Iimm_Print(rd, funct3, rs1, imm);
	} else if(opcode == 35){ //S-type
		uint32_t imm = (instruction & 3968) >> 7;
		uint32_t funct3 = (instruction & 28672) >> 12;
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t rs2 = (instruction & 32505856) >> 20;
		uint32_t imm2 = (instruction & 4261412864) >> 25;
		S_Print(imm2, funct3, rs1, rs2, imm);
	}  else if(opcode == 99){
		
		uint32_t funct3 = (instruction & 28672) >> 12;
		uint32_t rs1 = (instruction & 1015808) >> 15;
		uint32_t rs2 = (instruction & 32505856) >> 20;
		int32_t imm11 = (instruction & 128) >> 7;
		int32_t imm4_1 = (instruction & 3840) >> 7;
		int32_t imm10_5 = (instruction & 2113929216) >> 25;
		int32_t imm12 = (instruction & 2147483648) >> 31;
		int32_t imm = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1); //13 bits long
		if(imm12 == 1){
			imm = 0xFFFFE000 | imm;
		}
		B_Print(imm, funct3, rs1, rs2);

	} else if (opcode==115) {
		printf("ecall\n\n");
		RUN_FLAG = FALSE;
	} else{
		printf("instruction print not yet created\n");
		RUN_FLAG = FALSE;
	}
	CURRENT_STATE = NEXT_STATE;
	return;
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-RISCV SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	//B_Processing(7, 0, 19, 20, 10); // 7, 10 = 117

	
	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}