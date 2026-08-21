#include <stdint.h>
#include "config.h"
#include "syscall.h"

static inline int is_SVC_instruction(uint16_t instruction) {
    return (instruction & SVC_INST_ENCODING_MSK) == SVC_INST_ENCOGING;
}

void SVC_Handler_Main(ExceptionFrame_t* frame) {

    /*
  * Stack contains:
  * r0, r1, r2, r3, r12, r14, the return address and xPSR
  * First argument (r0) is frame->r0
  */
    uint16_t instruction = *(const uint16_t*)(frame->pc - 2U);
    if (!is_SVC_instruction(instruction)){
        frame->r0 = TINY_FAIL;
        return;
    }
        

    uint32_t svc_number = instruction & SVC_NUMBER_MSK;

    switch (svc_number) {
        case SVC_TASK_CREATE: {
            

            break;
        }
        default: /* unknown SVC */ break;
    }
}