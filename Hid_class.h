/*
 * Hid_class.h
 *
 * Created: 5/17/2011 4:45:00 PM
 *  Author: Gary Stofer
 */ 
#include "config.h"
#include "conf_usb.h"
#include "modules/usb/host_chap9/usb_host_enum.h"


#ifndef HID_CLASS_H_
#define HID_CLASS_H_


#define host_hid_set_idle()               (usb_request.bmRequestType = USB_SETUP_SET_CLASS_INTER,\
                                           usb_request.bRequest      = 0x0A,\
                                           usb_request.wValue        = 0,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = 0,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))

/**
 * @brief this function send the hid specific request "get hid report descriptor"
 *
 *
 * @param none
 *
 *
 * @return status
 */
#define host_get_hid_repport_descriptor() (usb_request.bmRequestType = USB_SETUP_GET_STAND_INTERFACE,\
                                           usb_request.bRequest      = 0x06,\
                                           usb_request.wValue        = 0x2200,\
                                           usb_request.wIndex        = 0,\
                                           usb_request.wLength       = SIZEOF_DATA_STAGE,\
                                           usb_request.uncomplete_read = FALSE,\
                                           host_send_control(data_stage))
#define ReportItemCount 20  // needs to be at least nJS_items

#ifdef USE_HID_PARSER
	enum item_Types		{ MAIN,GLOBAL,LOCAL};
	enum PageTypes		{ Pg_None=0,Pg_Generic,Pg_Button=0x9};//"Pg_None" is used to terminate the array						// All others are ignored
	enum GenericUsages	{ U_Joystick=0x4,U_X=0x30,U_Y,U_Z,U_Rx,U_Ry,U_Rz,U_Vy=0x41};				//		""
	enum ButtonUsages	{ U_btnNone=0,U_btn1,U_btn2,U_btn3,U_btn4,U_btn5,U_btn6,U_btn7,U_btn8};	//		""
	enum MainItemTags	{ Input=0x8,Collection=0xA,EndCollection=0xC};															//		""
	enum GlobalItemTags	{ UsagePage=0,ReportSz=0x7,ReportId,ReportCnt,Push,Pop};													//		""
	enum LocalItemTags	{ Usage=0,Usage_Min,Usage_Max};
	enum CollTypes		{ Coll_Physical,Coll_Application,Coll_Locical,Coll_Report};
	enum Report_items	{ Roll,Pitch,Yaw,Throttle,b1,b2,b3,b4,b5,b6,b7,b8,Knob,TrimSw,nJS_items};
		


	typedef struct
	{
		U8 offs;			  // Offset into report buffer of first byte
		U8 shift;			  // how many bits in the first byte are preceding the data 
		U8 n_bits;			  // total number of bits of the data
	}t_xlate_offs;

	
	typedef struct 			 // Global Items as per HID spec
	{
		U8 usage_page;
		U8 report_size;
		U8 report_count;
		U8 report_id;
	}t_Global_items;	 

	typedef struct 			// Local Items as per HID spec
	{
		signed char usg_fifo_ndx;	
		U8 usage_fifo[ReportItemCount];
	}t_Local_items;

 
	typedef struct  
	{								// Only Short items are supported -- HID class Joystick/Button Usage not using any long items 
		unsigned sz:2;				// size of data argument following this item tag
		enum item_Types type:2;		// 0=Main,1=Global,2=Local,3=Reserved
		unsigned tag:4;				// the key to what  the item is 
	
	} t_HidItemTag;
	extern t_xlate_offs X_tabl[nJS_items];
#else  // use report structure as learned 
	// structure of the data the interlink controllers send down the pipe as learned from the HID-report descriptor
	typedef struct
	{
		U8 ReportID;
		U8 roll;
		U8 pitch;
		U8 throttle;
		U8 knob;
		U8 yaw;
		U8 trimSW;
		U8 sw_1_8;
	
	} t_JS_USB_data;
#endif
extern bool Hid_Parse_ReportDesc(U8);

#endif /* HID_CLASS_H_ */