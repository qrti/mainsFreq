#include "sample.h"

sSample* Sample::samples;
uint32_t Sample::csi;                   
uint32_t Sample::cst;     
bool Sample::preShift = false;

uint32_t Sample::si;             

PIO Sample::pio_sample;   
uint Sample::sm_sample;    
pio_sm_config Sample::sm_config_sample;
uint Sample::sm_offset_sample;

uint Sample::dma_shift_chan;
dma_channel_config Sample::dma_shift_conf;

int16_t Sample::sum;
uint16_t Sample::cnt;
uint16_t Sample::dis;
uint16_t Sample::mis;

volatile int16_t Sample::iSum;
volatile uint16_t Sample::iCnt;
volatile uint16_t Sample::iDis;              
volatile uint16_t Sample::iMis;              

// 1 ms = 125e3 / 2     SM count cycles [smc] @ 125 MHz   
// 1 us = 125 / 2

#define TNOM    20000L                      // nominal mains cycle [us]
#define TRNG    200L                        // range               [us] 
#define TOUT    100L                        // timeout             [us]
#define SCTOP  (TNOM + TRNG) * 125L / 2L    // SM counter top      [smc]
#define SCRNG  (TRNG * 125L / 2L)           //            range    [smc]

void Sample::init()
{
    initSamples();

    initPio();
    initIrq();
    initDma();

    pio_sm_put(pio_sample, sm_sample, SCTOP);                                   // put counter top to TX buffer (once)
    pio_sm_set_enabled(pio_sample, sm_sample, true);                            // start state machine    
}

void Sample::initSamples()
{
    samples = (sSample*)malloc(sizeof(sSample) * NUMSAM);
}

void Sample::initPio()
{
    pio_sample = pio0;  

    gpio_set_dir(SAMPLE_PIN, GPIO_IN);
    // gpio_set_input_hysteresis_enabled(SAMPLE_PIN, false);
    gpio_disable_pulls(SAMPLE_PIN);

    pio_gpio_init(pio_sample, SAMPLE_PIN);                                      // init sample pin    

    sm_sample = pio_claim_unused_sm(pio_sample, true);                          // claim state machine
    sm_offset_sample = pio_add_program(pio_sample, &sample_program);            // load pio program to pio memory
    sm_config_sample = sample_program_get_default_config(sm_offset_sample);     // get default sm config

    sm_config_set_jmp_pin(&sm_config_sample, SAMPLE_PIN);                       // set jmp pin
    sm_config_set_in_shift(&sm_config_sample, false, false, 0);                 // shift in  dir, ISR -> RX FIFO, no autopush
    sm_config_set_out_shift(&sm_config_sample, false, false, 0);                //       out      TX FIFO -> OSR
    pio_sm_clear_fifos(pio_sample, sm_sample);                                  // clear FIFOs
    pio_sm_init(pio_sample, sm_sample, sm_offset_sample, &sm_config_sample);    // initialize pio state machine with config
}

void Sample::initIrq()
{    
    // map pio0 irq0 to cpu irq0     
    pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;                                    // irq0 enable source, pio_set_irq0_source_enabled(pio0, pis_interrupt0, true)           
    irq_set_exclusive_handler(PIO0_IRQ_0, pio_ir_handler_RX);                   //      set PIO RX as handler   
    irq_set_enabled(PIO0_IRQ_0, true);                                          //      enable
}

// range (display mid = 20e3 us)
// f nominal 50 Hz -> 20e3 us (ok ~ +-0.1 %, critical > +-0.4 % = +-80 us)
// -63 us -> 50.16 Hz -> f + 0.32 %  -> light  load -> graph below mid    (min display)
// +64    -> 49.84    ->   - 0.32    -> heavy       ->       above        (max       )
//
// mains        sm          -  SCRNG =   cycd [us]      load
// too fast     20200..301  -  200   =   20000..101     disturbance
// fast         300..201    -  200   =   100..1         light
// nominal      200         -  200   =   0              normal
// slow         199..100    -  200   =  -1..-100        heavy
// too slow     99..0       -  200   =  -101..-200      missing wave
//
void Sample::pio_ir_handler_RX() 
{
    static int32_t cycd;                    // cycle deviation

    if(pio0_hw->irq & 1){                   // pio_interrupt_get(pio0, 0) or pio0->irq & 0b1111 (?)
        pio0_hw->irq = 1;                   // pio_interrupt_clear(pio0, 0)

        #pragma optimize("", off)
        cycd = (((int32_t)pio_sm_get(pio_sample, sm_sample) - SCRNG) * 2L) / 125L;
        #pragma optimize("", on)

        if(cycd > TOUT){                    // disturbance
            iDis++;                         //
        }
        else if(cycd < -TOUT){              // missing wave
            iMis++;                         //
        }
        else{                               // sum up periods and count waves
            iSum += cycd;                   // 
            iCnt++;                         //
        }
    }
}

void Sample::initDma()
{
    dma_shift_chan = dma_claim_unused_channel(true);
    dma_shift_conf = dma_channel_get_default_config(dma_shift_chan);          

    channel_config_set_transfer_data_size(&dma_shift_conf, DMA_SIZE_16);    // sizeof(sSample) = 2 byte    
    channel_config_set_read_increment(&dma_shift_conf, true);
    channel_config_set_write_increment(&dma_shift_conf, true);
}

void Sample::shift()
{
    dma_channel_configure(dma_shift_chan, &dma_shift_conf,
                          samples,                          // write address
                          samples + 1,                      // read address
                          NUMSAM * sizeof(sSample),         // element count
                          true);                            // start 

    dma_channel_wait_for_finish_blocking(dma_shift_chan);
}

void Sample::read()
{           
    irq_set_enabled(PIO0_IRQ_0, false);
    sum = iSum;
    cnt = iCnt;
    dis = iDis;
    mis = iMis;
    iSum = iCnt = iDis = iMis = 0;     
    irq_set_enabled(PIO0_IRQ_0, true);
}

void Sample::store(uint32_t secs)
{
    read();

    if(secs < 3)
        return;

    csi = si;
    cst = secs;

    if(preShift)
        shift();

    samples[si].sample = cnt ? (sum / cnt) : 0;
    samples[si].status = (dis ? ERR_DIS : ERR_NONE) | (mis ? ERR_MIS : ERR_NONE);

    if(si < NUMSAM-1)
        si++;
    else
        preShift = true;
}
