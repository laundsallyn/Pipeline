  /*
 Simple Microprocessor (SM)            Cuong Chau & Warren A. Hunt, Jr.

 Version 0.7   circa April, 2016

  gcc -fno-asynchronous-unwind-tables -Wall -O2 -o sm simple-micro-0.7.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pipeline-ctrl-basic.c"

typedef unsigned short u4;
typedef unsigned short u8;
typedef unsigned short u16;
typedef short i16;

typedef unsigned int u32;
typedef int i32;

#define MAX_LINE_LEN 500
#define MAX_TOKEN_LEN 50

#define WIDTH      (16)
#define REGS       (16)
#define MEMSIZE    (65536)

#define BITS_4     (15)
#define BITS_8     (255)
#define BITS_12    (4095)
#define BITS_16    (65535)

u16 pop_count( u16 n )
{
  int i;
  u16 answer = 0;
  for( i = 0; i < WIDTH; i++ )
    {
      answer += ( n & 0x1 );
      n >>= 1;
    }
  return( answer );
}

u16 bit_reverse( u16 n )
{
  int i;
  u16 answer = 0;
  for( i = 0; i < WIDTH; i++ )
    {
      answer <<= 1;
      answer |= ( n & 0x1 );
      n >>= 1;
    }
  return( answer );
}

#define OP_SHIFT   (12)
#define REGC_SHIFT (8)
#define REGB_SHIFT (4)
#define REGA_SHIFT (0)
#define DATA_SHIFT (0)


// The state of SM.

i16  mem[ MEMSIZE ];
i16  reg[ REGS ];
u16  pc;

void micro_step( )
{
  u16 instruction = (u16) mem[ pc ];
  pc = (u16) pc + 1;         // Increment program counter

  u16 fn    = (u16) ((instruction >> 12) & BITS_4);
  u16 rnumc = (u16) ((instruction >>  8) & BITS_4);
  u16 rnumb = (u16) ((instruction >>  4) & BITS_4);
  u16 rnuma = (u16) ((instruction      ) & BITS_4);
  u16 data  = (u16) ((instruction      ) & BITS_8);
  u16 addr;

  i16 regc  = reg[ rnumc ];
  i16 regb  = reg[ rnumb ];
  i16 rega  = reg[ rnuma ];

  // Instruction-by-instruction debugging statement
  // printf(" %5d, %2d,  %3d,   %2d,   %2d,   %2d,  %5d,  %5d,  %5d.\n",
  //        pc-1, fn, data, rnumc, rnumb, rnuma, regc, regb, rega );

  switch ( fn ) {
  case  0:
    switch ( rnumb ) {
    case  0:                                               break;  // noop00        0

    case  1: reg[ rnumc ] = mem[ (u16) rega ];             break;  // ldmem         5

    case  2: mem[ (u16) regc ] = rega;                     break;  // stmem         6

    case  3: rega = rega - 1;                                      //               7
             reg[ rnuma ] = rega;
             mem[ (u16) rega ] = pc;
             pc = (u16) regc;                              break;  // call

    case  4: pc = (u16) mem[ (u16) rega ];                         //               8
             reg[ rnuma ] = rega + 1;                      break;  // return

    case  5: pc = (regc) ? ((u16) rega)      : pc;         break;  // jump          4
    case  6: pc = (regc) ? ((u16) rega) + pc : pc;         break;  // bra, branch   4

    case  7:                                               break;  // unassigned    0
    case  8:                                               break;  // unassigned    0

    case  9: reg[ rnumc ] = ~ rega;                        break;  // not           1
    case 10: reg[ rnumc ] = - rega;                        break;  // neg, negate   1
    case 11: reg[ rnumc ] = ! rega;                        break;  // cnot          1
    case 12: reg[ rnumc ] = pop_count( rega );             break;  // popcnt        1
    case 13: reg[ rnumc ] = bit_reverse( rega );           break;  // bitrev        1

    case 14: reg[ rnumc ] = mem[ (u16) rega ];                     //               9
             reg[ rnuma ] = rega + 1;                      break;  // pop

    case 15: addr = (u16) rega - 1;                                //              10
             mem[ addr ] = regc;
             reg[ rnuma ] = addr;                          break;  // push

    default:  break;
    }                                                      break;  // End of case 0
  case  1: reg[ rnumc ] =  regb  +  rega;                  break;  // add           2
  case  2: reg[ rnumc ] =  regb  -  rega;                  break;  // sub           2
  case  3: reg[ rnumc ] =  regb  *  rega;                  break;  // mul           2
  case  4: reg[ rnumc ] =  regb  /  rega;                  break;  // div           2

  case  5: reg[ rnumc ] =  regb  ^  rega;                  break;  // xor           2
  case  6: reg[ rnumc ] =  regb  &  rega;                  break;  // and           2
  case  7: reg[ rnumc ] =  regb  |  rega;                  break;  // lor           2

  case  8: reg[ rnumc ] =  regb << (rega & 0xF);           break;  // sleft         2
  case  9: reg[ rnumc ] =  regb >> (rega & 0xF);           break;  // sright        2

  case 10: reg[ rnumc ] =  regb  <  rega;                  break;  // lt            2
  case 11: reg[ rnumc ] =  regb <=  rega;                  break;  // lteq          2

  case 12: reg[ rnumc ] = (regb) ?  rega : regc;           break;  // cmove         3
  case 13: reg[ rnumc ] = (regb) ?  rega + regc : regc;    break;  // cadd          3

  case 14: reg[ rnumc ] = (regc & 0xFF00) | data;          break;  // immlow        3
  case 15: reg[ rnumc ] = (data << 8) | (regc & 0x00FF);   break;  // immhgh        3
  default: break;
  }
}


// Pipeline registers

typedef struct{
  u16  pc;       // PC value
} f_register;

typedef struct{
  u4   fn;       // function nibble
  u4   rnumc;    // rC
  u4   rnumb;    // rB or subfunction nibble
  u4   rnuma;    // rA
  u16  valP;     // incremented PC
} d_register;

typedef struct{
  u4   fn;       // function nibble
  u4   rnumc;    // rC
  u4   rnumb;    // rB or subfunction nibble
  u4   rnuma;    // rA
  i16  valC;     // value of register rC
  i16  valB;     // value of register rB
  i16  valA;     // value of register rA
  u8   data;     // immediate data
  u16  valP;     // incremented PC (same as valP in d_register)
} e_register;

typedef struct{
  u4   fn;       // function nibble
  u4   rnumc;    // rC
  u4   rnumb;    // rB or subfunction nibble
  u4   rnuma;    // rA
  i16  aluR;     // ALU result
  i16  valC;     // value of register rC
  i16  valA;     // value of register rA
  u16  valP;     // incremented PC (same as valP in e_register)
} m_register;

typedef struct{
  u4   fn;       // function nibble
  u4   rnumc;    // rC
  u4   rnumb;    // rB or subfunction nibble
  u4   rnuma;    // rA
  i16  aluR;     // ALU result
  i16  memV;     // memory value
  i16  valC;     // value of register rC
} w_register;

// Declare the pipeline registers.
f_register cF, nF;
d_register cD, nD;
e_register cE, nE;
m_register cM, nM;
w_register cW, nW;

i16 mux_2(int ctrl, i16 a, i16 b)
{
  if(!ctrl) return a;
  else return b;
}

i16 mux_3(int ctrl, i16 a, i16 b, i16 c)
{
  if(!ctrl) return a;
  else if(ctrl == 1) return b;
  else return c;
}

i16 mux_4(int ctrl, i16 a, i16 b, i16 c, i16 d)
{
  if(!ctrl) return a;
  else if(ctrl == 1) return b;
  else if(ctrl == 2) return c;
  else return d;
}

i16 alu(u4 fn, u4 rnumb,
	i16 rega, i16 regb, i16 regc, u8 data,
	u16 pc)
{
  i16 aluR = 0;

  switch ( fn ) {
  case  0:
    switch ( rnumb ) {
    case  0:                                      break;  // noop00      0
    case  1: aluR = rega;                         break;  // ldmem       5
    case  2: aluR = regc;                         break;  // stmem       6
    case  3: aluR = rega - 1;                     break;  // call        7
    case  4: aluR = rega + 1;                     break;  // return      8
    case  5: aluR = regc ? rega : pc;             break;  // jump        4
    case  6: aluR = regc ? rega + pc : pc;        break;  // bra, branch 4
    case  7:                                      break;  // unassigned  0
    case  8:                                      break;  // unassigned  0
    case  9: aluR = ~ rega;                       break;  // not         1
    case 10: aluR = - rega;                       break;  // neg, negate 1
    case 11: aluR = ! rega;                       break;  // cnot        1
    case 12: aluR = pop_count( rega );            break;  // popcnt      1
    case 13: aluR = bit_reverse( rega );          break;  // bitrev      1
    case 14: aluR = rega + 1;                     break;  // pop         9
    case 15: aluR = rega - 1;                     break;  // push       10
    default:  break;
    }                                             break;

  case  1: aluR =  regb  +  rega;                 break;  // add         2
  case  2: aluR =  regb  -  rega;                 break;  // sub         2
  case  3: aluR =  regb  *  rega;                 break;  // mul         2
  case  4: aluR =  regb  /  rega;                 break;  // div         2
  case  5: aluR =  regb  ^  rega;                 break;  // xor         2
  case  6: aluR =  regb  &  rega;                 break;  // and         2
  case  7: aluR =  regb  |  rega;                 break;  // lor         2
  case  8: aluR =  regb << (rega & 0xF);          break;  // sleft       2
  case  9: aluR =  regb >> (rega & 0xF);          break;  // sright      2
  case 10: aluR =  regb  <  rega;                 break;  // lt          2
  case 11: aluR =  regb <=  rega;                 break;  // lteq        2
  case 12: aluR = regb ? rega : regc;             break;  // cmove       3
  case 13: aluR = regb ? rega + regc : regc;      break;  // cadd        3
  case 14: aluR = (regc & 0xFF00) | data;         break;  // immlow      3
  case 15: aluR = (data << 8) | (regc & 0x00FF);  break;  // immhgh      3
  default: break;
  }

  return aluR;
}

// The state of the pipelined SM.

i16  pipe_mem[ MEMSIZE ];
i16  pipe_reg[ REGS ];
u16  pipe_pc;

// Memory access contants

#define MREAD  0
#define MWRITE 1
#define NO_MEM_ACCESS 2

// Fetch stage
void fetch()
{
  int pc_sel = pc_ctrl(cW.fn, cW.rnuma, cW.rnumb, cW.rnumc);
  int inst_sel = inst_ctrl(cD.fn, cD.rnuma, cD.rnumb, cD.rnumc);
  pipe_pc = mux_4(pc_sel, cF.pc, cW.aluR, cW.memV, cW.valC);
  u16 instruction = mux_2(inst_sel, pipe_mem[ pipe_pc ], 0x0070);
  if(!inst_sel) {
    pipe_pc++;

  }
  // Update the nD register
  nD.fn    = (instruction >> 12) & BITS_4;
  nD.rnumc = (instruction >>  8) & BITS_4;
  nD.rnumb = (instruction >>  4) & BITS_4;
  nD.rnuma = (instruction      ) & BITS_4;
  nD.valP  = pipe_pc;

  // Update the nF register
  nF.pc = pipe_pc;
  //pc not incremented

}  

// Decode stage
void decode()
{
  nE.fn = cD.fn;
  nE.rnumc = cD.rnumc;
  nE.rnumb = cD.rnumb;
  nE.rnuma = cD.rnuma;
  nE.valC = pipe_reg[cD.rnumc];
  nE.valB = pipe_reg[cD.rnumb];
  nE.valA = pipe_reg[cD.rnuma];
  nE.data = (cD.rnumb << 4) | cD.rnuma;

  nE.valP = cD.valP;
}

// Execute stage
void execute()
{
  nM.fn = cE.fn;
  nM.rnumc = cE.rnumc;
  nM.rnumb = cE.rnumb;
  nM.rnuma = cE.rnuma;

  nM.aluR = alu(cE.fn, cE.rnumb,
		cE.valA, cE.valB, cE.valC, cE.data,
		cE.valP);

  nM.valC = cE.valC;
  nM.valA = cE.valA;
  nM.valP = cE.valP;
}

// Memory stage
void memory()
{
  int mem_access = mem_access_ctrl(cM.fn, cM.rnuma, cM.rnumb, cM.rnumc);
  int addr_sel = addr_ctrl(cM.fn, cM.rnuma, cM.rnumb, cM.rnumc);
  int memInput_sel = memInput_ctrl(cM.fn, cM.rnuma, cM.rnumb, cM.rnumc);

  nW.fn = cM.fn;
  nW.rnumc = cM.rnumc;
  nW.rnumb = cM.rnumb;
  nW.rnuma = cM.rnuma;
  nW.aluR = cM.aluR;

  u16 addr = mux_2(addr_sel, cM.aluR, cM.valA);
  if(mem_access == MREAD)
    nW.memV = pipe_mem[addr];  
  else if(mem_access == MWRITE)
    pipe_mem[addr] = mux_3(memInput_sel, cM.valA, cM.valC, cM.valP);

  nW.valC = cM.valC;
}

// Write-back stage
void write_back()
{
  int reg_sel = reg_ctrl(cW.fn, cW.rnuma, cW.rnumb, cW.rnumc);
  int mem_wb = mem_wb_ctrl(cW.fn, cW.rnuma, cW.rnumb, cW.rnumc);
  int alu_wb = alu_wb_ctrl(cW.fn, cW.rnuma, cW.rnumb, cW.rnumc);

  if(mem_wb)
    pipe_reg[cW.rnumc] = cW.memV;
  if(alu_wb) 
    pipe_reg[mux_2(reg_sel, cW.rnuma, cW.rnumc)] = cW.aluR;
 }

// Step the pipe by one clock cycle.
void pipe_step()
{
  // Run the five stages on the current pipe register values.
  fetch();
  decode();
  execute();
  memory();
  write_back();

  // Update the current pipe registers with their next value.
  cF = nF;
  cD = nD;
  cE = nE;
  cM = nM;
  cW = nW;
}

// Initialize current pipeline registers.
void init_pipeline_regs() {
  // F register
  cF.pc = 0;

  // D register
  cD.fn = 0;
  cD.rnumc = 0;
  cD.rnumb = 0;
  cD.rnuma = 0;
  cD.valP = 0;

  // E register
  cE.fn = 0;
  cE.rnumc = 0;
  cE.rnumb = 0;
  cE.rnuma = 0;
  cE.valC = 0;
  cE.valB = 0;
  cE.valA = 0;
  cE.data = 0;
  cE.valP = 0;

  // M register
  cM.fn = 0;
  cM.rnumc = 0;
  cM.rnumb = 0;
  cM.rnuma = 0;
  cM.aluR = 0;
  cM.valC = 0;
  cM.valA = 0;
  cM.valP = 0;

  // W register
  cW.fn = 0;
  cW.rnumc = 0;
  cW.rnumb = 0;
  cW.rnuma = 0;
  cW.aluR = 0;
  cW.memV = 0;
  cW.valC = 0;
}

// Compare routine

int compare_ISA_to_pipeline_prog_state () {
  unsigned int i;

  if ( pc != pipe_pc ) {
    printf( "ERROR:  pc is:  %d, pipe_pc is: %d.\n", pc, pipe_pc );
    int a;
    for(a = 0; a < 16; a += 2)
      printf("reg[%d] = %d   reg[%d] = %d\n",a, reg[a], a+1, reg[a+1]);
    printf("\n");
    for(a = 0; a < 16; a += 2)
      printf("pipe_reg[%d] = %d   pipe_reg[%d] = %d\n",a, pipe_reg[a], a+1, pipe_reg[a+1]);
      
    return( 0 );
  }

  for( i = 0; i < REGS; i++ )
    if ( reg[ i ] != pipe_reg[ i ] ) {
      printf( "ERROR:  reg[ %d ] is:  %d, pipe_reg[ %d ] is: %d.\n", i, reg[i], i, pipe_reg[i] );
      int a;
      for(a = 0; a < 16; a += 2)
        printf("reg[%d] = %d   reg[%d] = %d\n",a, reg[a], a+1, reg[a+1]);
      printf("\n");
      for(a = 0; a < 16; a += 2)
        printf("pipe_reg[%d] = %d   pipe_reg[%d] = %d\n",a, pipe_reg[a], a+1, pipe_reg[a+1]);
      return( 0 );
    }

  for( i = 0; i < MEMSIZE; i++ )
    if ( mem[ i ] != pipe_mem[ i ] ) {
      printf( "ERROR:  mem[ %d ] is:  %d, pipe_mem[ %d ] is: %d.\n", i, mem[i], i, pipe_mem[i] );
      exit( 0 );
    }

  return( 1 );
}

void print_sm_state() {
  unsigned int i;

  printf( "Program Counter (PC),       %6d,   HEX:  %4x\n\n",
	  (u16) pc, (u16) pc );

  printf( "Reg number, Integer, Natural,  Hex\n" );
  for ( i = 0; i < REGS; i++ )
    printf( "Reg  %4d:  %7d, %7u,  %4x\n", i, reg[ i ], (u16) reg[ i ], (u16) reg[ i ] );

  printf( "\n" );

  printf( "Mem address, Integer, Natural,  Hex\n" );
  for ( i = 0; i < 54; i++ )
    printf( "Mem  %6d: %7d, %7u,  %4x\n", i, mem[ i ], (u16) mem[ i ], (u16) mem[ i ] );
}


int main (int argc, char *argv[], char *env[] )
{

  #define MAXLINELEN (1000)
  char buf[ MAXLINELEN ];
  long int i = 0;
  u32 address;
  int value_at_address;
  long int count;       // Number of ISA-level instructions to execute
  long int pipe_count;  // Number of pipeline-level cycles to execute

  if ( argc != 4 ) {
    printf( "Three input arguments: <n> <p> <filename>.\n" );
    printf( "where <n> is a positive number of ISA-level instructions to execute,\n" );
    printf( "where <p> is a positive number of pipeline-level cycles to execute, and\n" );
    printf( "<filename> is a file containing lines with address-value pairs.\n\n" );

    printf( "Example:   sm  10  20  initial-memory.txt\n" );

    printf( "means to load the SM ISA-level memory and the pipeline-level memory\n");
    printf( "with the contents of file \"initial-memory.txt\n -- and then run the\n" );
    printf( "SM ISA-level emulator for 10 steps, run the SM pipeline-level emulator,\n" );
    printf( "compare the state of the ISA-level and pipeline-level emulator, and\n");
    printf( "finally compare the programmer-visible state of the two emulations.\n" );
    exit( 1 );
    }

  // Initialize count
  count = strtol( argv[1], (char **)NULL, 10);

  if ( errno != 0 ) {
    printf( "Error when reading <n>.\n" );
    exit( errno );
  }

  if ( count < 0 ) {
    printf( "ISA-level count is negative, SM emulator terminated.\n" );
    exit( 1 );
  }

  // Initialize pipe_count
  pipe_count = strtol( argv[2], (char **)NULL, 10);

  if ( errno != 0 ) {
    printf( "Error when reading <p>.\n" );
    exit( errno );
  }

  if ( pipe_count < 0 ) {
    printf( "Pipeline-level count is negative, SM emulator terminated.\n" );
    exit( 1 );
  }

  // Open file for the program.
  FILE *file = fopen( argv[ 3 ], "r" );

  if ( file == NULL ) {
    printf( "%s not found.\n", argv[ 3 ] );
    exit( 1 );
  }

  // Read and display input arguments...

  printf( "Max number of SM Instructions to execute: %ld.\n", count );

  // Initialize all memory locations and all registers
  for ( i = 0; i < MEMSIZE - 1; i++ ) {
    mem[ i ] = 0;
    pipe_mem[ i ] = 0;
  }

  for ( i = 0; i < REGS; i++ ) {
    reg[ i ] = 0;
    pipe_reg[ i ] = 0;
  }

  pc = 0;  // Note:  Initial pc is 0.
  pipe_pc = 0;

  init_pipeline_regs(); // Initialize current pipeline registers.

  while( !feof( file ) ) {
    if ( fgets( buf, MAXLINELEN-1, file ) != NULL )
      sscanf( buf, "%u %i", &address, &value_at_address );
    if ( ! ( address < 65536 ) ) {
      printf( "Out of range address: %u.\n", address );
      exit( 2 );
    }
    if ( ! ( -32768 <= value_at_address && value_at_address < 65536 ) ) {
      printf( "Out of range value at address: %u, %d.\n", address, value_at_address );
      exit( 2 );
    }

    mem[ address ] = (i16) value_at_address;
    pipe_mem[ address ] = (i16) value_at_address;
  }

  fclose( file );

  // Put some instructions in the memory...
  // I compile to x86 with the following command:
  //   gcc -fno-asynchronous-unwind-tables -O2 -S <prog_to_compile.c>


  // !!! The next line is the header for the debugging (print) statement
  // !!! in the "micro_step" procedure.  If you uncomment the next line,
  // !!! then it makes sense to uncomment the line in "micro_step".

  // printf("    pc, fn, data,   rc,   rb,  ra,    regc,   regb,   rega.\n" );

  // Finally, run the program...
// int b, result;
// for(b = 0; b < count; b++){
  for ( i   = 0; i < count; i++ ){
    printf("pc = %d\n",pc);
      micro_step( );
  }

  for ( i = 0; i < pipe_count; i++ ){
    pipe_step( );
    if(!((!cD.fn) && (cD.rnumb == 7))) {
      printf("pc = %d \n",cF.pc - 1);
      printf("code is %d %d %d %d \n", cD.fn, cD.rnumc,cD.rnumb,cD.rnuma);
     }
  }
  int result = compare_ISA_to_pipeline_prog_state( );
  printf("\nnumber of pipline instructions executed = %ld\n",i);

  // Output final RAX value.

  // printf( "Final value of R0 is: %d.\n", reg[ 0 ] );

  // The following statements produce additional simulator output...

  // printf( "\n" );
  // printf( "Instructions Executed:  %lu\n\n", count );

  /* printf( "Instruction Read  Cost:  %9ld\n", inst_rd_cnt ); */
  /* printf( "       Data Read  Cost:  %9ld\n", data_rd_cnt ); */
  /* printf( "       Data Write Cost:  %9ld\n", data_wr_cnt ); */

// Again, a mechanism to debug your program.  The routine
// print_sm_state can be changes to print more or less, or maybe
// changed so that it also prints memory locations starting at 4096!

//  print_sm_state(); // Print partial state.

  exit( !result );
}
