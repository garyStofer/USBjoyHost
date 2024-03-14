/*
** ============================================================================
**
** FILE
**  main.c
**
** DESCRIPTION
**  This is the main file of the project.	    
**  HW platform EZLink module with C8051F93x Silabs MCU 
**
** CREATED
**  Silicon Laboratories Hungary Ltd
**
** COPYRIGHT
**  Copyright 2009 Silicon Laboratories, Inc.  
**	http://www.silabs.com
**
** ============================================================================
*/


								/* ======================================================== *
								 *									INCLUDE					*
							 	 * ======================================================== */


#include "C8051F930_defs.h"
#include "compiler_defs.h"
#include "string.h"
#include "si4432.h"
#include "Setup_Si4432_RX.h"


							/* ================================================================ *
							 * C8051F930 Pin definitions for Software Development Board         *
							 * 					(using compiler_def.h macros) 				    *
							 *	================================================================*/	

SBIT (NSS, SFR_P1, 3);
SBIT (NIRQ, SFR_P0, 6);
SBIT (SDN, SFR_P0, 1);

SBIT (LEDGrn,  SFR_P2, 0);      		   //  Used to output A serco PWM    
SBIT (PCM_out, SFR_P1, 4);                 //  Used to output PCM train , AKA GPIO-04
SBIT (OUTput0, SFR_P1, 5);
SBIT (LEDRed,  SFR_P1, 6);                 //  Used to output A serco PWM    


SBIT (PB,     SFR_P0, 7); 

	
#define PWM_NEUT	0x7d	// creates a 1.5Ms signal
#define PWM_MAX		0xf4	// creates a 1.9762Ms signal
#define PWM_MIN		0x06	// creates a 1.024 Ms signal
			
#define TRIM_DELAY 20
#define TRIM_MAX 50


enum PCM_CH { Thr=0,Roll,Pitch,Yaw,Knob,Sw1,Sw2,Sw3,Sw4,NUM_PCM_CH};
static unsigned  char  PCMchannel[NUM_PCM_CH] = {PWM_MIN,0x7d,0x7d,0x7d,0x7d,0,0,0,0};		// variable channnel time in 4us increments       


enum RFpayload_ch { THROTTLE=0,ROLL,PITCH,YAW,KNOB,SW1_8,TRM_SW,MAX_RF_PAYLOAD};
static unsigned char payload[MAX_RF_PAYLOAD] = {0x00,0x7d,0x7d,0x7d,0x7d,0x00,0x00};

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  +
  + FUNCTION NAME:  void MCU_Init(void)
  +
  + DESCRIPTION:   	This function configures the MCU 
  + 		
  + INPUT:			None
  +
  + RETURN:         None
  +
  + NOTES:          None
  +
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void 
MCU_Init(void)
{
	unsigned char tmp;
	
	//Disable the Watch Dog timer of the MCU
	PCA0MD   &= ~0x40;                  

	
	tmp = OSCICL ;
	OSCICL = tmp+2;			// set internal precision Osc to 24Mhz
	OSCICN = 0x8f;			// enable internal precision Osc

	while ( !(OSCICN & 0x40)) 	// while not running at programmed frequency "IFRDY bit"
	; 

	CLKSEL = 0x10;				// switch to precision osc. /2  Sysclock = 24mhz/2 == 12mhz

	while ( !(CLKSEL & 0x80 ))	// wait for "CLKRDY bit"
	;


	// Initialize the the IO ports and the cross bar
	P0SKIP  |= 0x40;                    // skip P0.6
	XBR1    |= 0x40;                    // Enable SPI1 (3 wire mode)
	
	P1MDOUT |= 0x01;                    // Enable SCK push pull
	P1MDOUT |= 0x04;                    // Enable MOSI push pull
	P1SKIP  |= 0x08;                    // skip NSS
	P1MDOUT |= 0x08;                    // Enable NSS push pull
	P1SKIP  |= 0x40;                    // skip LEDRed
	P1MDOUT |= 0x50;                    // Enable Red & Yel LED push pull
	
	P2SKIP  |= 0x01;                    // skip LEDGrn
	P2MDOUT |= 0x01;                    // Enable LEDGrn push pull
	P0SKIP  |= 0x02; 				    // skip SDN
	P0MDOUT |= 0x02;					// Enable SDN push pull
	P0SKIP  |= 0x80;                    // skip PB
	SFRPAGE  = CONFIG_PAGE;
	P0DRV    = 0x12;                  	// TX high current mode
	P1DRV    = 0x4D;                   	// MOSI, SCK, NSS, LEDRed high current mode
	P2DRV	 = 0x01;					// LEDGrn high current mode	
	SFRPAGE  = LEGACY_PAGE;
	XBR2    |= 0x40;                    // enable Crossbar

	// For the SPI communication the hardware peripheral of the MCU is used 
	//in 3 wires Single Master Mode. The select pin of the radio is controlled
	//from software
	SPI1CFG   = 0x40;					// Master SPI, CKPHA=0, CKPOL=0
	SPI1CN    = 0x00;					// 3-wire Single Master, SPI enabled
	SPI1CKR   = 0x00;
	SPI1EN 	 = 1;                     	// Enable SPI interrupt
	NSS = 1;

	// Turn off the LEDs
	LEDRed=0;
	LEDGrn=0;
	PCM_out=0;
}


void 
Timer0_Init(void)
{
   TH0 = 0xff;           // some start values
   TL0 = 0x00 ;          // ""
   TMOD = 0x01;                        // Timer0 in 16-bit mode
   CKCON = 0x02;                       // Timer0 uses a 1:48 prescaler  1 count == 4us when sysclock == 12Mhz
   ET0 = 1;                            // Timer0 interrupt enabled
   TCON = 0x10;                        // Timer0 ON
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  +
  + FUNCTION NAME: void SpiWriteRegister(U8 reg, U8 value)
  +
  + DESCRIPTION:   This function writes the registers 
  + 					
  + INPUT:			 U8 reg - register address   
  +					 U8 value - value write to register	
  +
  + RETURN:        None
  +
  + NOTES:         Write uses a Double buffered transfer
  +
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void SpiWriteRegister (U8 reg, U8 value)
{
   // Send SPI data using double buffered write

	//Select the radio by pulling the nSEL pin to low
   NSS = 0;                            
   
   //write the address of the register into the SPI buffer of the MCU
	//(important to set the MSB bit)
	SPI1DAT = (reg|0x80);				//write data into the SPI register
	//wait until the MCU finishes sending the byte
	while( SPIF1 == 0);					
	SPIF1 = 0;
	//write the new value of the radio register into the SPI buffer of the MCU
	SPI1DAT = value;						//write data into the SPI register
	//wait until the MCU finishes sending the byte
	while( SPIF1 == 0);					//wait for sending the data
	SPIF1 = 0;	       
   //Deselect the radio by pulling high the nSEL pin
	NSS = 1;                            

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  +
  + FUNCTION NAME: U8 SpiReadRegister(U8 reg)
  +
  + DESCRIPTION:   This function reads the registers 
  + 					
  + INPUT:			 U8 reg - register address   
  +
  + RETURN:        SPI1DAT - the register content 
  +
  + NOTES:         none
  +
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
U8 SpiReadRegister (U8 reg)
{

	//Select the radio by pulling the nSEL pin to low
	NSS = 0;                            
	//Write the address of the register into the SPI buffer of the MCU 
	//(important to clear the MSB bit)
    SPI1DAT = reg;								//write data into the SPI register
	//Wait untill the MCU finishes sending the byte
	while( SPIF1 == 0);						
	SPIF1 = 0;
	//Write a dummy data byte into the SPI buffer of the MCU. During sending 
	//this byte the MCU will read the value of the radio register and save it 
	//in its SPI buffer.
	SPI1DAT = 0xFF;							//write dummy data into the SPI register
	//Wait untill the MCU finishes sending the byte
	while( SPIF1 == 0);						
	SPIF1 = 0;
	//Deselect the radio by pulling high the nSEL pin
	NSS = 1;                            

	//Read the received radio register value and return with it
   return SPI1DAT;
}

//-----------------------------------------------------------------------------
// Timer0_ISR
//-----------------------------------------------------------------------------
//
// Here we process the Timer0 interrupt and toggle the PCM output_pin according to the channel data received
//
//-----------------------------------------------------------------------------
#define TIMER_CLOCK 4
#define PULSE_REST_time  	(400/TIMER_CLOCK)
#define PULSE_BASE_time 	(600/TIMER_CLOCK)
#define MIN_PAUSE_time 		(5000/TIMER_CLOCK)

   
INTERRUPT (TIMER0_ISR, INTERRUPT_TIMER0)
{
	static unsigned char ch_n = 0;		// increments through the PCMchannel[] to fetch the current PWM count 
	static unsigned  rest =  0;			// State flag, indicates that the "rest" pulse is to be generated next
	short pulse_time;					// Var to calculate the count for the timer 

	if (rest++ )		// This is the fixed time rest pulse between individual channels
	{
		PCM_out = 0;

		rest = 0;
		pulse_time = PULSE_REST_time; 	// 400 us "rest" pulse, separating individual channels from each other 
		
		// outputting PWM (servo) CH on pins
		if (ch_n == YAW )  // yaw
			LEDRed = 1;
		else
			LEDRed = 0;
		
		if (ch_n == THROTTLE ) // throttle
			LEDGrn = 1;
		else
			LEDGrn = 0;
				
	}
	else 
	{
		PCM_out = 1;
		
		if (ch_n >= NUM_PCM_CH)			// create the frame sync pulse >= 5ms for receiver sync 
		{

			// add up the time of all channels in this frame and set the Pause to be the remainder to ~22ms 
			pulse_time = 20000/TIMER_CLOCK; // 20ms . 
			while (ch_n )
			{

				pulse_time -= (PCMchannel[--ch_n] + PULSE_BASE_time + PULSE_REST_time);

			}

			if ( pulse_time < MIN_PAUSE_time)	// make at least a 5ms sync pulse 
				pulse_time = MIN_PAUSE_time;

		}
		else 	// calculate the pulse width for the current channel
			pulse_time = PCMchannel[ch_n++] + PULSE_BASE_time; // 600 us is base pulse width, base pulse + rest time = 1ms 
		
	}

	pulse_time = -pulse_time; 				// timer0 counts up to 0xffff 
  	TH0 = ((pulse_time & 0xff00) >>8);	 	// set new count value
    TL0 = pulse_time & 0xff;

}



void 
main(void)
{
	U8 ItStatus1,ItStatus2, status3;
	U16 delay;
	int trim_delay =0;
	volatile short temp;
	//volatile unsigned short thr;
	U8 length;
	static signed char throttle_trim,roll_trim, pitch_trim,yaw_trim;
	

	//Initialize the MCU: 
	//	- set IO ports for the Software Development board
	//	- set MCU clock source
	//	- initialize the SPI port 
	//	- turn ofF LEDs
	MCU_Init();

	Timer0_Init();
	EA = 1;                             // Enable global interrupts

	/* ================================================ *
	 *			Initialize the Si443x radio  chip		* 
	 * ================================================ */
	PCM_out = 0;
	LEDGrn = 1;
	LEDRed = 1;

	//Turn on the radio by pulling down the PWRDN pin
	SDN = 0;  	 // this will trigger a power on Reset -- should take 16ms to occur -- Nirq should go active 

	for(delay=0;delay<60000;delay++)		// wait out the Power on Reset time (~16ms) without talking to the device -- SPI is not ready 
	{
		if ( NIRQ == 0 )					// device has completed power on reset and issued POR interrupt 
				break;
	}
	 

	do 	// wait for ichiprdy				
	{
		ItStatus2 = SpiReadRegister(InterruptStatus2 );
		if (delay++ > 60000)
			break;

	}while (! (ItStatus2 & 0x2) ) 	;			// ichiprdy 
	 
	if (delay >60000 ) 
	{

		while(1);	//error in communication with radio device don't continue
 	}

	//--------------  end of radio enabling sequence 
	PCM_out = 0;
	LEDGrn = 0;
	LEDRed = 0;

	throttle_trim=roll_trim=pitch_trim=yaw_trim= -5;  // shift 4x4us counts since Joystick neutral is at 128 and PWM neutral is at 125  
		
	//read interrupt status registers to clear the interrupt flags and release NIRQ pin
	ItStatus1 = SpiReadRegister(InterruptStatus1);		//read the Interrupt Status1 register
	ItStatus2 = SpiReadRegister(InterruptStatus2);		//read the Interrupt Status2 register
						
	//SW reset   
  	SpiWriteRegister(OperatingFunctionControl1, 0x80);

	//wait for POR interrupt from the radio (while the nIRQ pin is high)
	delay=0;
	while ( NIRQ == 1)
	{
		delay++;
	}
	  
	//read interrupt status registers to clear the interrupt flags and release NIRQ pin
	ItStatus1 = SpiReadRegister(InterruptStatus1);	
	ItStatus2 = SpiReadRegister(InterruptStatus2);	

	//wait for chip ready interrupt from the radio (while the nIRQ pin is high)
	delay=0;
	while ( NIRQ == 1)
	{ 
		delay++;
	}
	
	//read interrupt status registers to clear the interrupt flags and release NIRQ pin
	ItStatus1 = SpiReadRegister(InterruptStatus1);	
	ItStatus2 = SpiReadRegister(InterruptStatus2);	
	
	ItStatus1 = SpiReadRegister(DeviceType );		 
	ItStatus2 = SpiReadRegister(DeviceVersion);


	//set  Crystal Oscillator Load Capacitance register & rx-tx switch control
	SpiWriteRegister(CrystalOscillatorLoadCapacitance, 0xD7);	//write 0xD7 to the CrystalOscillatorLoadCapacitance register
   	SpiWriteRegister(GPIO1Configuration, 0x12);	//write 0x12 to the GPIO1 Configuration(set the TX state)
	SpiWriteRegister(GPIO2Configuration, 0x15);	//write 0x15 to the GPIO2 Configuration(set the RX state) 

	/*Configure the receive packet handler*/
	//Enable the receive packet handler and CRC-16 (IBM) check
	SpiWriteRegister(DataAccessControl, 0x85);
	// set the header bytes that should be evaluated for the header OK status	           
	SpiWriteRegister(HeaderControl1, 0x0C);	//enable header  bytes 4 and 4 to be checked 		
	SpiWriteRegister(HeaderControl2, 0x22); 	// two header bytes and two Sync bytes

   	//Disable header bytes; set variable packet length (the length of the payload is defined by the
	//received packet length field of the packet); set the synch word to two bytes long
	//set the preamble length to 10 nibbles i.e.  40 bits   
	SpiWriteRegister(PreambleLength, 0x0A);				//write 0x0A to the Preamble Length register == 40 bits

	//Set the sync word pattern to 0x2DD4
	SpiWriteRegister(SyncWord3, 0x2d);				//write 0x2D to the Sync Byte 3 register
	SpiWriteRegister(SyncWord2, 0xD4);				//write 0xD4 to the Sync Byte 2 register
	
	SpiWriteRegister(CheckHeader3, 0xCA);				//write Header Byte register
	SpiWriteRegister(CheckHeader2, 0xFE);
	//Set the Heder check enable bits  (which bits are supposed to be compared )
	SpiWriteRegister(HeaderEnable3, 0xff);																
	SpiWriteRegister(HeaderEnable2, 0xff);				//write Header Byte 2 register

//set the center frequency to 915 MHz
// what about 73 & 74
	SpiWriteRegister(FrequencyBandSelect, Si4432_FREQUENCY_BAND);             
 	SpiWriteRegister(NominalCarrierFrequency1, Si4432_NOMINAL_CARRIER_FREQUENCY_1);		//write to the Nominal Carrier Frequency1 register
 	SpiWriteRegister(NominalCarrierFrequency0, Si4432_NOMINAL_CARRIER_FREQUENCY_0);  	//write to the Nominal Carrier Frequency0 register
	SpiWriteRegister(FrequencyHoppingStepSize, Si4432_FREQUENCY_HOPPING_STEP_SIZE); 
	SpiWriteRegister(FrequencyHoppingChannelSelect, Si4432_FREQUENCY_HOPPING_CHANNEL);

// set the modem parameters according to WDS3 calcs 
	SpiWriteRegister(IFFilterBandwidth, 				Si4432_IF_FILTER_BANDWIDTH);			  
	SpiWriteRegister(ClockRecoveryOversamplingRatio , 	Si4432_CLOCK_RECOVERY_OVERSAMPLING_RATIO);
	SpiWriteRegister(ClockRecoveryOffset0, 				Si4432_CLOCK_RECOVERY_OFFSET_0);		  
	SpiWriteRegister(ClockRecoveryOffset1, 				Si4432_CLOCK_RECOVERY_OFFSET_1);		  
	SpiWriteRegister(ClockRecoveryOffset2, 				Si4432_CLOCK_RECOVERY_OFFSET_2);		  	
	SpiWriteRegister(ClockRecoveryTimingLoopGain1, 		Si4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_1);	
	SpiWriteRegister(ClockRecoveryTimingLoopGain0, 		Si4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_0);

	SpiWriteRegister(AFCLoopGearshiftOverride, 			Si4432_AFC_LOOP_GEARSHIFT_OVERRIDE);	
	SpiWriteRegister(AFCTimingControl, 					Si4432_AFC_TIMING_CONTROL);				
	SpiWriteRegister(ClockRecoveryGearshiftOverride, 	Si4432_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE); 
	SpiWriteRegister(AFClimit, 							Si4432_AFC_LIMIT);						
	SpiWriteRegister(AGCOverride1, 						Si4432_AGC_OVERRIDE_1);					

	SpiWriteRegister(ModulationModeControl1, Si4432_MODULATION_MODE_CONTROL_1);
	SpiWriteRegister(ModulationModeControl2, Si4432_MODULATION_MODE_CONTROL_2 | 0x20);	 // Oddly enogh -- The modulation Source FIFO must must be selected even though we are in receive mode 
	SpiWriteRegister(FrequencyDeviation, Si4432_FREQUENCY_DEVIATION);

									

   /*enable receiver chain*/
	SpiWriteRegister(OperatingFunctionControl1, 0x5);	// RX_on 	
	/*
	3.2.2.4. READY Mode  -- Bit 0
READY Mode is designed to give a fast transition time to TX mode with reasonable current consumption. In this
mode the Crystal oscillator remains enabled reducing the time required to switch to TX or RX mode by eliminating
the crystal start-up time. READY mode is entered by setting xton = 1 in "Register 07h. Operating Mode and
Function Control 1". To achieve the lowest current consumption state the crystal oscillator buffer should be
disabled in Register 62h. Crystal Oscillator Control and Test.To exit READY mode, bufovr (bit 1) of this register
must be set back to 0.
*/	


	//Enable two interrupts: 
	// a) one which shows that a valid packet received: 'ipkval'
	// b) second shows if the packet received with incorrect CRC: 'icrcerror' 
	SpiWriteRegister(InterruptEnable1, 0x03); 		//write 0x03 to the Interrupt Enable 1 register
	SpiWriteRegister(InterruptEnable2, 0x00); 		//write 0x00 to the Interrupt Enable 2 register

	//read interrupt status registers to release all pending interrupts
	ItStatus1 = SpiReadRegister(InterruptStatus1);	//read the Interrupt Status1 register
	ItStatus2 = SpiReadRegister(InterruptStatus2);	//read the Interrupt Status2 register

	/*MAIN Loop*/
// LEDRed and LEDGreen are uses for PWM servo outputs in the interrupt function	
	while(1)
	{

		
//This indicates a Header check error 
		status3 = SpiReadRegister(DeviceStatus);
		if (status3 & 0x10 )
		{
		//	LEDRed = 1;
		//	PCMchannel[4] = 0x7f;
// need to decide what to do on a failed package
		}
			
	    //wait for the interrupt event
		if( NIRQ == 0 )
		{
			
			//LEDGrn = 1; //to indicate PKG receipt

			//read interrupt status registers 
			ItStatus1 = SpiReadRegister(InterruptStatus1);	
			ItStatus2 = SpiReadRegister(InterruptStatus2);	

			//disable the receiver chain 
			SpiWriteRegister(OperatingFunctionControl1, 0x01);//Ready mode, Xtal stays on for 200us turnaround to RX again 										
			
			/*CRC Error interrupt occured*/
			if( ItStatus1 & 0x01) 
			{
				//blink red LEDs to show the error
			//	LEDRed = 1;
			//	PCMchannel[4] = 0x7f;// Causes an error condition in UAVP  since CH5 can only be on or off 
			}	/*packet received interrupt occured*/
			else if( ItStatus1 & 0x02 )
			{

				//Read the length of the received payload
				length = SpiReadRegister(ReceivedPacketLength);
				
				//check whether the received payload is not longer than the allocated buffer in the MCU
				if(length <= MAX_RF_PAYLOAD)
				{
					//Get the received payload from the RX FIFO
					for(temp=0;temp < length;temp++)
					{
						payload[temp] = SpiReadRegister(FIFOAccess);
					}
					

					// process the trim switches -- could be done in the transmitter as well, but if receiver has local 
					// eeprom storage, the trim could be stored there. ( not implemented)
					if (payload[TRM_SW] != 0 )		// one of the trim switches pressed
					{	
						if ( trim_delay-- <= 0)		// wait out some time for increment of trim if button stays pressed
						{
							trim_delay = TRIM_DELAY;		// start a delay for next repeat 
							switch (payload[TRM_SW])
							{
								case 2:
									if (roll_trim < TRIM_MAX)
										roll_trim++;
									break;

								case 1:	
									if ( roll_trim > -TRIM_MAX)	
										roll_trim--;
									break;

								case 6:
									if (pitch_trim < TRIM_MAX)
										pitch_trim++;
									break;

								case 3:	
									if ( pitch_trim > -TRIM_MAX)	
										pitch_trim--;
									break;

								case 18:
									if (yaw_trim < TRIM_MAX)
										yaw_trim++;
									break;

								case 9:	
									if ( yaw_trim > -TRIM_MAX)	
										yaw_trim--;
									break;

								case 54:
									if (throttle_trim < TRIM_MAX)
										throttle_trim++;
									break;

								case 27:	
									if ( throttle_trim > -TRIM_MAX)	
										throttle_trim--;
									break;

							} // end switch
						} // end if 
					
					}
					else 
						trim_delay = 0;

				
	

	// for now invert the polarity of the signals here, but it could also be done in the transmitter
	// together with the neutral stick offset calibration. 


					temp =  0xFF - payload[THROTTLE] - throttle_trim;				// invert by substracting from 0xff
					PCMchannel[Thr] = (temp>PWM_MAX)?PWM_MAX:(temp<PWM_MIN)?PWM_MIN:temp;		 
					
					temp =  /*0xff -*/ payload[ROLL] - roll_trim;				 
					PCMchannel[Roll] = (temp>PWM_MAX)?PWM_MAX:(temp<PWM_MIN)?PWM_MIN:temp;		 

					temp = 0xFF - payload[PITCH] - pitch_trim;						// invert 
					PCMchannel[Pitch] = (temp>PWM_MAX)?PWM_MAX:(temp<PWM_MIN)?PWM_MIN:temp;		
					
					temp = /*0xff - */(payload[YAW] - yaw_trim);								
					PCMchannel[Yaw] = (temp>PWM_MAX)?PWM_MAX:(temp<PWM_MIN)?PWM_MIN:temp;		 

					temp = /*0xff - */ payload[KNOB];
					PCMchannel[Knob] = (temp>PWM_MAX)?PWM_MAX:(temp<PWM_MIN)?PWM_MIN:temp;		

					PCMchannel[Sw1] = (payload[SW1_8] &0x1) ? PWM_MAX : PWM_MIN; 		// Left 2way Switch
					PCMchannel[Sw2] = (payload[SW1_8] &0x2) ? PWM_MAX : PWM_MIN; 		// Right 2way switch
					PCMchannel[Sw3] = (payload[SW1_8] &0x4) ? PWM_MAX : PWM_MIN; 		// Red Reset Button 
					PCMchannel[Sw4] = (payload[SW1_8] &0x8) ? PWM_MAX : PWM_MIN;	    // Left 3way switch pos down 


				}	
			
			}
			//reset the RX FIFO
   	   		SpiWriteRegister(OperatingFunctionControl2, 0x02);	
        	SpiWriteRegister(OperatingFunctionControl2, 0x00);	
			//enable the receiver chain again
			SpiWriteRegister(OperatingFunctionControl1, 0x05);	
		} 	
	}
}

