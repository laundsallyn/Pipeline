// Yipu Wang yw6483
// April 2016
#include <stdio.h>
typedef unsigned short u4;

// Memory access contants

#define MREAD  0
#define MWRITE 1
#define NO_MEM_ACCESS 2

// Below are the control functions for the pipelined SM. You are asked
// to implement these functions. You are allowed to change the inputs
// and outputs of these functions if needed. In that case, you also
// need to modify the pipeline stage functions defined in
// simple-micro-0.7.c since those functions call the control
// functions.

// Show your work here.
unsigned counter = 3;
// pc_ctrl don't need to use ruma, and rumc
int pc_ctrl(u4 fn, u4 rnuma, u4 rnumb, u4 rnumc)
{
   switch (fn){
    case 0:
    switch ( rnumb ) {
      case  3: return 3;
      case  4: return 2;
      case  5: 
      case  6: return 1;
      default: return 0;
    }
    default: return 0;
  }
}

int inst_ctrl(u4 fn, u4 rnuma, u4 rnumb, u4 rnumc)
{
	if(counter == 3){
		counter = 0;
		return 0;
	}else {
    counter++;
		return 1;
  }
}

int mem_access_ctrl(u4 fn, u4 rnuma, u4 rnumb, u4 rnumc)
{
    switch (fn){
    case 0:
      switch (rnumb){
        case  1: return 0;
        case  2: 
        case  3: return 1;
        case  4:    
        case 14: return 0;
        case 15: return 1;
        default: return 2;
      }

    default: return 2;
  } 
}

int addr_ctrl(u4 fn, u4 rnuma, u4 rnumb, u4 rnumc)
{
  switch (fn){
    case 0:
      switch (rnumb){
        case  4: 
        case 14: return 1;    
        default: return 0;
      }
    default: return 0;
  }   
}

int memInput_ctrl(u4 fn, u4 rnuma, u4 rnumb, u4 rnumc)
{
    switch (fn){
    case 0:
      switch (rnumb){
        case  2: return 0; 
        case  3: return 2;
        case  15: return 1; 
        default: return 0;
      }
    default: return 0;
  }   
}

int reg_ctrl(u4 fn, u4 rnuma, u4 rnumb, u4 rnumc)
{
    switch (fn){
    case 0:
      switch (rnumb){
        case  3: 
        case  4:
        case 14:
        case 15: return 0;
        default: return 1;
      }
    default: return 1;
    }   
}

int mem_wb_ctrl(u4 fn, u4 rnuma, u4 rnumb, u4 rnumc)
{
  switch (fn){
  case 0:
    switch (rnumb){
      case 1: 
      case 14: return 1;
      default: return 0;
    }
  default: return 0;
  }
}

int alu_wb_ctrl(u4 fn, u4 rnuma, u4 rnumb, u4 rnumc)
{
  switch (fn){
  case 0:
    switch (rnumb){
      case  0: 
      case  1:
      case  2:
      case  5:
      case  6:
      case  7:
      case  8: return 0;
      default: return 1;
    }
  default: return 1;
  }
}
