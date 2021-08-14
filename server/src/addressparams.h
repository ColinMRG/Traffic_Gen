#define OFFSET_MEM_WRITE 0xa000000 // Memory write offset
#define OFFSET_S2MM_DMACR 0x30 // S2MM DMA control register
#define OFFSET_S2MM_DMASR 0x34 // S2MM DMA status register
#define OFFSET_S2MMDA 0x48 // S2MM destination address
#define OFFSET_S2MM_LENGTH 0x58 // S2MM buffer length


#define NUM_OF_WORDS 100
#define BUF_LEN      600000
#define FRAME_COUNT_MAX 1
#define TCP_MAX     2048
#define XAXIDMA_SR_IDLE_MASK 0x00000002


int wait_for_dma_done();
int dma_recvd_length();
