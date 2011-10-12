/*
 * CProgram1.c
 *
 * Created: 8/1/2011 1:30:22 PM
 *  Author: gary
 

 */ 

#include "config.h"
#include "Si4432.h"
#include "lib_mcu/spi/spi_lib.h"


U8 RFM22_SpiReadRegister (U8 reg)
{
	volatile U8 dat ;

	//Select the radio by pulling the nSEL pin to low
	RFM22_SEL();                           
	//Write the address of the register into the SPI buffer of the MCU 
	//(important to clear the MSB bit)
 //dat = DDRB;
 //dat = PRR0;	
 //dat = SPCR;
 //dat = SPSR;


   Spi_write_data(reg & 0x7F);
   Spi_write_data(0xff);								// clock out an other byte to generate the cloks for the read
   dat = Spi_get_byte();
	
	
	//dat = spi_getchar ();									//Read the received radio register value and return with it
	//De-select the radio by pulling high the nSEL pin
	RFM22_DESEL();
	
   return dat;
}


void RFM22_SpiWriteRegister (U8 reg, U8 value)
{
   // Send SPI data using double buffered write

	//Select the radio by pulling the nSEL pin to low
    RFM22_SEL();                           
   
    //write the address of the register into the SPI buffer of the MCU
	Spi_write_data(reg |= 0x80);	  // Address location in radio for write
	Spi_write_data(value);			  // Write data
	
	RFM22_DESEL();                          

}


void 
RFM22_Spi_Burst_Xfer( unsigned char read_WRITE, unsigned char addr, unsigned char  * data_ptr, unsigned char lenght)
{
	 
	if ( read_WRITE ) 
		addr |= 0x80;	// add in the write bit	
	else
		addr &= 0x7f; 		// clear write bit in address should it be set 
		
	
	RFM22_SEL(); 			// SPI CS
	Spi_write_data(addr);
				
	while (lenght--)			
	{
		Spi_write_data(*data_ptr);
		*data_ptr++ = Spi_get_byte() ; 	// capture output from device's miso
	}

	RFM22_DESEL();  // CS off
}


void
RFM22_Si4432_clr_interrupt_status( void )
{
	RFM22_SpiReadRegister (InterruptStatus1 );
	RFM22_SpiReadRegister (InterruptStatus2 );
}

void
RFM22_Si4432_init( void)
{
	volatile U16 delay;	
	volatile unsigned char IStatus;
	
//	RFM22_BUS_INIT();
	DDRD |= (1<<RFM22_NSEL_BIT );		// output 
	DDRD &= ~(1<<RFM22_NIRQ_BIT);		// input 
	RFM22_DESEL();
	
 	//spi_init (SPI_MASTER);
	DDRB |= 0x1;		// PB0 is nSS  // must be output for SPI master mode
 	Spi_init_bus();		// makes PB1( CLK) and PB2(MOSI) outputs
	SPCR = 0x50;		// Master mode SPI enable in full speed 
	
	for(delay=1;delay;delay++)			// wait for the radio to be powered up, ~16ms without talking to the device -- SPI is not ready 
	{
		if ( RFM22_NIRQ == 0 )					// device has completed power on reset and issued POR interrupt 
				break;
	}
	
	//read interrupt status registers to clear the interrupt flags and release NIRQ pin
	RFM22_Si4432_clr_interrupt_status();

	//SW reset   
  	RFM22_SpiWriteRegister(OperatingFunctionControl1, 0x80);					//write reset to the Operating & Function Control register 
	// fires two interrupts in sequence POR and CHPready
	// wait for interrupt from the radio (while the nIRQ pin is high)
	// Radio goes into IDLE READY mode after SW-RESET
	

	for(delay=1;delay;delay++)			// wait for the radio to be powered up, ~16ms without talking to the device -- SPI is not ready 
	{
		IStatus = RFM22_SpiReadRegister(InterruptStatus2 );
		if (IStatus & 0x2)				// ichiprdy 				// device has completed reset and issued POR interrupt 
			break;
	}
	
	IStatus = RFM22_SpiReadRegister(DeviceType);					// Device ID 			== 8 == EZRADIO_PRO
	IStatus = RFM22_SpiReadRegister(DeviceVersion);				// Device VERSION 		== 6 == B.1
	
		//set Crystal Oscillator Load Capacitance register
	RFM22_SpiWriteRegister(CrystalOscillatorLoadCapacitance, Si4432_CRYSTAL_OSCILLATOR_LOAD_CAPACITANCE);				//write 0x7f to the CrystalOscillatorLoadCapacitance register
	
	//set the GPIO's function	  
   	RFM22_SpiWriteRegister(GPIO1Configuration, 0x12);				//write 0x12 to the GPIO1 Configuration(set the TX state)
	RFM22_SpiWriteRegister(GPIO2Configuration, 0x15);				//write 0x15 to the GPIO2 Configuration(set the RX state) 
	

	//enable the TX packet handler and CRC-16  check
	RFM22_SpiWriteRegister(DataAccessControl, 0x0D);	//write 0x0D to the Data Access Control register

	//Disable header bytes; set variable packet length (the length of the payload is defined by the
	//received packet length field of the packet); set the synch word to two bytes long
	//SpiWriteRegister(0x32, 0x00);					// Header Control1 register
	RFM22_SpiWriteRegister(HeaderControl2, 0x22); 	// two header bytes and two Sync bytes
	
	/*set the packet structure and the modulation type*/
	//set the preamble length to 10 nibbles i.e.  40 bits   
	RFM22_SpiWriteRegister(PreambleLength, 0x0A);			//write 0x0A to the Preamble Length register == 40 bits


	//Set the sync word pattern to 0x2DD4
	RFM22_SpiWriteRegister(SyncWord3, 0x2d);				//write 0x2D to the Sync Byte 3 register
	RFM22_SpiWriteRegister(SyncWord2, 0xD4);				//write 0xD4 to the Sync Byte 2 register

	RFM22_SpiWriteRegister(TransmitHeader3, 0xCA);				//write Header Byte 3 register
	RFM22_SpiWriteRegister(TransmitHeader2, 0xFE);				//write Header Byte 2 register
		
	//set the TX power to MAX
//	SpiWriteRegister(0x6D, 0x07);				//write 0x1F to the TX Power register -- WDS3 sets this to 0x1f for max power 0x1e for -3db , 0x1d for -6db , etc..
	RFM22_SpiWriteRegister(TXPower, Si4432_TX_POWER );					//write 0x1F to the TX Power register 
	//set the desired TX data rate (9.6kbps)
	RFM22_SpiWriteRegister(TXDataRate1, Si4432_TX_DATA_RATE_1);			    //write 0x4E to the TXDataRate 1 register
	RFM22_SpiWriteRegister(TXDataRate0, Si4432_TX_DATA_RATE_0);				//write 0xA5 to the TXDataRate 0 register
	
	//enable FIFO mode and GFSK modulation
	RFM22_SpiWriteRegister(ModulationModeControl1, Si4432_MODULATION_MODE_CONTROL_1) ;	//write 0x2C to the Modulation Mode Control 1 register
	RFM22_SpiWriteRegister(ModulationModeControl2, Si4432_MODULATION_MODE_CONTROL_2);	//write 0x23 to the Modulation Mode Control 2 register  data from FIFO, GFSK

	//set the desired TX deviation 
	RFM22_SpiWriteRegister(FrequencyDeviation, Si4432_FREQUENCY_DEVIATION );


	//set the center frequency to 915 MHz
//	SpiWriteRegister(0x75, 0x75);				//write 0x75 to the Frequency Band Select register             
//	SpiWriteRegister(0x76, 0xBB);				//write 0xBB to the Nominal Carrier Frequency1 register
//	SpiWriteRegister(0x77, 0x80);  		   		//write 0x80 to the Nominal Carrier Frequency0 register


// setting up channel hopping and channel index with base (ch0) frequency at 915
	RFM22_SpiWriteRegister(FrequencyBandSelect, Si4432_FREQUENCY_BAND );		// High band select
	RFM22_SpiWriteRegister(NominalCarrierFrequency1, Si4432_NOMINAL_CARRIER_FREQUENCY_1);		// Fo
	RFM22_SpiWriteRegister(NominalCarrierFrequency0, Si4432_NOMINAL_CARRIER_FREQUENCY_0 );		// fo
	RFM22_SpiWriteRegister(FrequencyOffset1,  Si4432_FREQUENCY_OFFSET_1  );
	RFM22_SpiWriteRegister(FrequencyOffset2,  Si4432_FREQUENCY_OFFSET_2  );
	RFM22_SpiWriteRegister(FrequencyHoppingStepSize, Si4432_FREQUENCY_HOPPING_STEP_SIZE ); 	// 0x48 == 720khz spacing 
	RFM22_SpiWriteRegister(FrequencyHoppingChannelSelect, Si4432_FREQUENCY_HOPPING_CHANNEL);			// CH =0
	
 // SET IDLE mode IDLE READY + PLLON for faster turnaround to TX time 
 //	RFM22_SpiWriteRegister(Si4432_OperatingFunctionControl1, 0x3);	
}
