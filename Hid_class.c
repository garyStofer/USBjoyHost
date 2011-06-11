/*
 * Hid_class.c 
 * Created: 5/17/2011 4:43:16 PM
 *  Author: Gary Stofer
 */ 
#include "Hid_class.h"

#define G_STACK_SZ 3
static t_Global_items gloItmDat[G_STACK_SZ];

static t_Local_items  locItmDat;
t_xlate_offs X_tabl[nJS_items];




/* Partial implementation of a Hid_report parser.  Only parses the items required for an embedded use of a HID joystick or gamepad
 * Items of interest are the UsagePage/UsageTypes associated with a joystick device as well as the ReportSize and ReportCounts thereof
   No concern  is given to logical and physical limits nor to units and exponents. All axis are being reported as 8 bit values (reduced to 8bit if needed) 
   The output of the parser is a data structure that defines the type,size and location of the devices reporting values in the report buffer,
   so that the application layer can interpret the input pipe data correctly for any HID compliant joystick device.
   See www.usb.org/developers/devclass_docs/HID1_11.pdf chapter 6.2.2
*/

bool
Hid_Parse_ReportDesc(U8 ifc)
{
	bool isJoystickDevice = false;
	enum item_Types i_type;
	U8 sz;
	U8 i_tag;
	U8 u_min;
	U8 *end_p;
	U16 dat;
	U8 i,ii;
	t_HidItemTag *item;	
	S8 u_ndx =0;		// usage index
	U8 g_ndx;			// Global item index -- aka stack pointer
	U16 hid_repSize ;
	U8 bit_cnt =0;
	U8 u_type;


	
	hid_repSize = 	usb_tree.device[selected_device].interface[ifc].hid_rep_sz;
	host_get_hid_repport_descriptor();		// reads data_stage[] -- entire size of array hopefully enough
	
	item =(void*) data_stage;
	end_p = (void *) data_stage;
	end_p += hid_repSize;
	
	// init structs
	
	for (g_ndx =0;g_ndx<G_STACK_SZ;g_ndx++)
	{
		gloItmDat[g_ndx].report_count = 0;
		gloItmDat[g_ndx].report_id = 0;
		gloItmDat[g_ndx].report_size=  0;
		gloItmDat[g_ndx].usage_page = 0;
	}
	
	for (i=0;i<nJS_items;i++)
	{	
		X_tabl[i].n_bits= 0;		// item is not available in the data stream id mask is 0
	}
	
	u_ndx = g_ndx = locItmDat.usg_fifo_ndx =0;
	
	do 	 // over entire report descriptor data or until we run out of space in the translation table
	{
		i_tag = item->tag;
		i_type = item->type;
		sz = (item->sz==3) ? 4 : item->sz;	// sz==3 measn 4 bytes
				
		if (sz==1) 
			dat = *(U8 *) (item+1);
		else if (sz==2)
			dat = *(U16 *) (item+1);
		else 
			dat=0;	
		
		switch (i_type)
		{
			case MAIN:
				if (i_tag == Input)
				{
					 // build the translate_map[] from the Usage fifo and report size & counts
					if (!isJoystickDevice )		//Only interested in joystick devices
						break;
					
					// for each report item pertaining to this input token	
					
					for (ii=i=0; i < gloItmDat[g_ndx].report_count; i++)
					{
						if (locItmDat.usg_fifo_ndx != 0)	// no usage associated with it -- nothing to record these are fill bits 
						{
							u_type = locItmDat.usage_fifo[ii];
						
							u_ndx =-1;			// none 
					
							// filter out the usages we are interested in
							switch (gloItmDat[g_ndx].usage_page )
							{
								case Pg_Generic:
									switch (u_type)
									{
										case U_X:
											u_ndx= Roll;
											break;
										case U_Y:
											u_ndx=Pitch;
											break;
										
										case U_Rz:
										case U_Ry:
											u_ndx=Yaw;
											break;
											
										case U_Z:
										case U_Slider:
											u_ndx=Throttle;
											break;
										case U_Hat:
											u_ndx=Hat;
											break;
									}
									break;
								
								case Pg_Button:
									if (u_type>= U_btn1 && u_type <= U_btn12)
									{
										u_ndx = b1+u_type-1;  // -1 since there is no usage Button0								}
									}
									break;
							}									
							// store  the extraction parameters for the desired items 							
							if (u_ndx!= -1 && u_ndx < nJS_items)
							{
								//n_bits = gloItmDat[g_ndx].report_size; 
								X_tabl[u_ndx].offs =  bit_cnt / 8;
								X_tabl[u_ndx].shift = bit_cnt % 8;
								X_tabl[u_ndx].n_bits = gloItmDat[g_ndx].report_size; 
							}
	
							if (ii < locItmDat.usg_fifo_ndx-1)		// increment through the usages until we reach the end 
								ii++;								// should there be more report counts than usages in the report, the last usage is repeated
						}
						bit_cnt += gloItmDat[g_ndx].report_size; 
					}	// end for report count						
				}					
				else if (i_tag == Collection )
				{	// This checks to see that the attached device is a Joystick or Game-pad 
 				
					if (locItmDat.usg_fifo_ndx!=0)
						i = locItmDat.usage_fifo[locItmDat.usg_fifo_ndx-1];	// top most Usage if there was one
					else 
						i=0;
							
					if ( gloItmDat[g_ndx].usage_page == Pg_Generic && dat == Coll_Application )
					{
						if (i == U_Joystick || i == U_Gamepad)
							isJoystickDevice = true;
					} 
				}
				else if (i_tag == EndCollection)
				{
								
				}
				//all Main items clear all local items after processing
				locItmDat.usg_fifo_ndx =0;					
			break;
			
			case GLOBAL:
				if (i_tag == UsagePage)
					gloItmDat[g_ndx].usage_page = dat;
				else if (i_tag == ReportSz)
					gloItmDat[g_ndx].report_size =dat;
				else if (i_tag == ReportId)
				{	
					// If the report Descriptor contained a report ID it is always the first 8bits in a report.
					gloItmDat[g_ndx].report_id = dat;
					bit_cnt = 8;			// This is to skip over the report ID value if there is one given
				}					
				else if (i_tag == ReportCnt)
					gloItmDat[g_ndx].report_count =dat;
				else if (i_tag == Push)
				{
					if (g_ndx<G_STACK_SZ)
						g_ndx++;
				}					
				else if ( i_tag == Pop)
				{  if (g_ndx > 0)
						g_ndx--;		
				}					
			break;
			
			case LOCAL:
				if (i_tag == Usage)
				{
					if ( locItmDat.usg_fifo_ndx < ReportItemCount )			// Overflowing usage fifoDon't record any more
						locItmDat.usage_fifo[locItmDat.usg_fifo_ndx++] = dat;
				}
				else if(i_tag == Usage_Min)
					u_min= dat;
				else if (i_tag == Usage_Max)			  // relies on that Min comes before MAX
				{
					for(i=u_min; i<=dat;i++)
					{
						if ( locItmDat.usg_fifo_ndx < ReportItemCount )		// Overflowing usage fifo ?
							locItmDat.usage_fifo[locItmDat.usg_fifo_ndx++] = i;
					}
				}
			break;
		}	

		item+=sz+1;	// now pointing to next item tag
	} while ((U8 *)item <  end_p)	;	
	 
	return isJoystickDevice;
};	