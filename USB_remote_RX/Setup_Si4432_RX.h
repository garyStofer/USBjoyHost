
/**************************************************************
*** Configuration File generated automatically by           ***
*** SILICON LABS's Wireless Development Suite               ***
*** WDS GUI Version: 3.1.3.0                                    ***
*** Device: Si4432 Rev.: B1                                 ***
***                                                         ***
***                                                         ***
*** For further details please consult with the device      ***
*** datasheet, available at <http://www.silabs.com>         ***
***************************************************************/

/**************** Frequency Control ********************

        Operating Frequency: 915.000 MHz
 *******************************************************/

#define Si4432_CRYSTAL_OSCILLATOR_LOAD_CAPACITANCE 0x7F
/*
        Address:                0x09
        Crystal Oscillator Load Capacitance: 12.500 pF
*/

#define Si4432_FREQUENCY_OFFSET_1                  0x00
/*
        Address:                0x73
*/

#define Si4432_FREQUENCY_OFFSET_2                  0x00
/*
        Address:                0x74
*/

#define Si4432_FREQUENCY_BAND                      0x75
/*
        Address:                0x75
*/

#define Si4432_NOMINAL_CARRIER_FREQUENCY_1         0xBB
/*
        Address:                0x76
*/

#define Si4432_NOMINAL_CARRIER_FREQUENCY_0         0x80
/*
        Address:                0x77
*/

#define Si4432_FREQUENCY_HOPPING_STEP_SIZE         0x48
/*
        Address:                0x7a
        If frequency hopping is used then the step size should 
        be set first, and then the hopping channel, because the 
        frequency change occurs when the channel number is set.
*/

#define Si4432_FREQUENCY_HOPPING_CHANNEL           0x00
/*
        Address:                0x79
        If frequency hopping is used then the step size should 
        be set first, and then the hopping channel, because the 
        frequency change occurs when the channel number is set.
*/

/**************** TX Modulation Options ****************/

#define Si4432_TX_POWER                            0x1E
/*
        Address:                0x6d
*/

#define Si4432_TX_DATA_RATE_1                      0x20
/*
        Address:                0x6e
*/

#define Si4432_TX_DATA_RATE_0                      0xC5
/*
        Address:                0x6f
*/

#define Si4432_TX_DR_IN_BPS 128006L

#define Si4432_MODULATION_MODE_CONTROL_1           0x0C
/*
        Address:                0x70
*/

#define Si4432_MODULATION_MODE_CONTROL_2           0x03
/*
        Address:                0x71
Data Clock Config.: No TX Data Clock (only for OOK and FSK) 
Data Source:        FIFO Mode
Modulation Type:    FSK/GFSK

*/

#define Si4432_FREQUENCY_DEVIATION                 0x66
/*
        Address:                0x72
        Deviation: 63.75 kHz
*/

/**************** RX Modem settings ********************/

#define Si4432_IF_FILTER_BANDWIDTH            0x83
/*
        Address:                0x1c
*/

#define Si4432_AFC_LOOP_GEARSHIFT_OVERRIDE         0x40
/*
        Address:                0x1d
*/

#define Si4432_AFC_TIMING_CONTROL                  0x0A
/*
        Address:                0x1e
*/

#define Si4432_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE   0x03
/*
        Address:                0x1f
*/

#define Si4432_CLOCK_RECOVERY_OVERSAMPLING_RATIO   0x5E
/*
        Address:                0x20
*/

#define Si4432_CLOCK_RECOVERY_OFFSET_2             0x01
/*
        Address:                0x21
*/

#define Si4432_CLOCK_RECOVERY_OFFSET_1             0x5D
/*
        Address:                0x22
*/

#define Si4432_CLOCK_RECOVERY_OFFSET_0             0x8B
/*
        Address:                0x23
*/

#define Si4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_1   0x05
/*
        Address:                0x24
*/

#define Si4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_0   0x7A
/*
        Address:                0x25
*/

#define Si4432_AFC_LIMIT                           0x50
/*
        Address:                0x2a
*/

#define Si4432_AGC_OVERRIDE_1                      0x60
/*
        Address:                0x69
*/

/**************** Operation mode ***********************/

#define Si4432_OPERATING_AND_FUNCTION_CONTROL_1    0x04
/*
        Address:                0x07
        Operation Mode: Rx
*/

/**************** End of Header file *******************/

