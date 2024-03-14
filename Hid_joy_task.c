//_____  I N C L U D E S ___________________________________________________

#include "config.h"
#include "string.h"
#include "conf_usb.h"
#include "hid_class.h"
#include "Si4432.h"
#include "modules/usb/host_chap9/usb_host_task.h"
#include "modules/usb/host_chap9/usb_host_enum.h"
#include "lib_mcu/usb/usb_drv.h"

// These are the differences between the neutral position of the sticks and the neutral values for the 1.5ms PWM 
// PWM neutral is 1.5Ms or 0x7d ( 125)
#define Roll_offset (-7)
#define Pitch_offset (-4)
#define Yaw_offset (-4)

U8  device_connected=0;

U8  HID_Device_PipeIn;

#ifdef USE_HID_PARSER
	U8 JS_Data[nJS_items];
#endif	
 
U8 SPI_buffer[16];

void Hid_JoyStickTaskInit(void)
{
	volatile U8 i;


	Leds_init();
#ifdef USE_HID_PARSER	  
	for (i=0;i<nJS_items;i++)
	{
		JS_Data[i] = 0;
	}
#endif	

  
	RFM22_Si4432_init();
}

// See VID_PID table in conf_USB.h for controllers that are enumerated 

void Hid_joyStickTask(void)
{

	S8 n_bits;
	U8 rep_len;
	U8 i;
	U16 dat16;
	U8 buff[ReportItemCount];
	U8 ndx; 

	//Test HID device connection
	if(Is_host_ready())
	{
		if(Is_new_device_connection_event())   //Device connection
		{
			device_connected=0;

			for(i=0;i<Get_nb_supported_interface();i++)
			{
				// this seems  redundant since we only deal with HID devices  in the enumeration to begin with
				if(Get_class(i)== HID_CLASS  ) 
				{
					host_hid_set_idle();
				  
					if ((device_connected = Hid_Parse_ReportDesc(i))!= 0 )
					{
						// connect pipe and get polling running -- i.e start getting data from the device
						HID_Device_PipeIn=host_get_hwd_pipe_nb(Get_ep_addr(i,0));   // HOW do we know which EP we want ???????
						Host_select_pipe(HID_Device_PipeIn);
						Host_continuous_in_mode();
						Host_unfreeze_pipe();
					}
					break;			// Simply using the first interface of HID class and endpoint 0 of it 
				}
			}
		}
		if(device_connected)
		{
			Host_select_pipe(HID_Device_PipeIn);
			
			if(Is_host_in_received())
			{
				if( !Is_host_stall() )
				{
					rep_len = Host_data_length_U8();
					if (rep_len >= ReportItemCount)
							rep_len  = ReportItemCount;
							
					for(ndx=0; ndx<rep_len; ndx++)	// copy the data out of the device pipe for processing
						buff[ndx] = Host_read_byte();
#ifdef USE_HID_PARSER

						
					for(ndx=0; ndx < nJS_items ; ndx++)		// go through the map and pick out the vars from the buffer
					{
						if ( (n_bits = X_tabl[ndx].n_bits)== 0)	// Item not in report
							continue;
							
						dat16 =  buff[X_tabl[ndx].offs]	;		// read item from report buffer	lsb
						dat16 += buff[X_tabl[ndx].offs+1]<<8; 	// msb
						
				
						dat16 <<= 16 - n_bits - X_tabl[ndx].shift;			  // top align
						dat16 >>= 16-n_bits;				 // shift down -- masking out bottom bits
						
						if (n_bits > 8)						// reduce all data items to 8 bits -- i.e. Logitek 3D is 10 bit on X and Y
							dat16 >>= (n_bits -8);			 
						
						JS_Data[ndx] = (U8) dat16;
												
					}	
					

					// send new data via radio
					//SET THE CONTENT OF THE radio PACKET -- same channel order as JR RC control 
					SPI_buffer[0] = JS_Data[Throttle];
					SPI_buffer[1] = JS_Data[Roll] + Roll_offset;
					SPI_buffer[2] = JS_Data[Pitch] +Pitch_offset;
					SPI_buffer[3] = JS_Data[Yaw] + Yaw_offset;
					SPI_buffer[4] = JS_Data[Knob];
					SPI_buffer[5] = JS_Data[b1] | JS_Data[b2]<<1 | JS_Data[b3]<<2 | JS_Data[b4]	<<3 |JS_Data[b5]<<4 | JS_Data[b6]<<5 | JS_Data[b7]<<6 | JS_Data[b8]<<7 ;
					SPI_buffer[6] = JS_Data[TrimSw];
					
					// new data to report locally via LEDS
					if (JS_Data[b3] != 0)	// if the red reset button is pressed
						Led3_on();
					else
						Led3_off();
					
#else  // Not using the HID parser -- instead read the USB data directly by the struct learned from the HID_report via USB analyzer
					t_JS_USB_data * usb_js_data = (void * ) buff;
					SPI_buffer[0] = usb_js_data->throttle;
					SPI_buffer[1] = usb_js_data->roll; 
					SPI_buffer[2] = usb_js_data->pitch;
					SPI_buffer[3] = usb_js_data->yaw;  
					SPI_buffer[4] = usb_js_data->knob; 
					SPI_buffer[5] = usb_js_data->sw_1_8;
					SPI_buffer[6] = usb_js_data->trimSW;
					
					if (usb_js_data->sw_1_8 & 0x4 ) //  if the red reset button is pressed indicate on LED
						Led3_on();
					else
						Led3_off();

#endif					
					Led1_on();	   // flash LED during transmit			
					RFM22_Spi_Burst_Xfer(SPI_WRITE, FIFOAccess, SPI_buffer, 7);	//transfer the data into the transmit fifo		

					//set the length of the payload to n bytes	
					RFM22_SpiWriteRegister(0x3E, 7);			
		
					//Disable all other interrupts and enable the packet sent interrupt only.
					//This will be used for indicating the success full packet transmission for the MCU
			
					RFM22_SpiWriteRegister(InterruptEnable1, 0x04);					//Enable ipkgsent 	
					RFM22_SpiWriteRegister(InterruptEnable2, 0x00);					//
			
					RFM22_Si4432_clr_interrupt_status();										

					/*enable transmitter*/
					//The radio forms the packet and sends it automatically.
					RFM22_SpiWriteRegister(OperatingFunctionControl1 , 0x09);
					// RFM22_SpiWriteRegister(OperatingFunctionControl1 , 0x0B);
					// or possibly 0xB for faster TX turn around time by leaving the PLL running and staying in TUNE mode
				

					// upon ipksent interrupt the device goes back into Idle Ready mode as  programmed in 0x7, Si4432_OperatingFunctionControl1 
				
					//The MCU just needs to wait for the 'ipksent' interrupt.
					while(RFM22_NIRQ == 1)
						;
					// Radio is now back in IDLE mode as programmed in reg 0x7, Si4432_OperatingFunctionControl1 	
					Led1_off();	
						

							   		  
				} // end !Is_host_stall()
			
			   	Host_ack_in_received();
				Host_send_in();
			} // end host_in_recieved
// TODO: Not sure what this does here -- seems to be executing each time through 			
		/*
			if(Is_host_nak_received())
			{
				Host_ack_nak_received();
				Led0_off(); Led1_off();
				Led2_off(); Led3_off();
			}
		 */
		} // end if isDevice connected
   }
   //Device disconnection...
   if(Is_device_disconnection_event())
   {
      Leds_off();
      device_connected=0;
   }
}
