This folder contains the ATMEL studio project for the USB /stick RF transmitter  and a sub project named "USB_remote_RX" implemented with the SI-labs 8051 based controller.

The Atmel studio IDE resides on my old VISTA computer and is named  USBjoyHost 
The SI-Lab IDE for the 8051 and the tools to configure the RF chip (RFM22) is found on the old Dell2400 XP machine.

The transmitter side ( ATMEL) connects via USB to the Interlink controller from Great Planes RC flight simulator and reads the stick and buttons via it's USB interface.
Initially coded with a HID device parser, the current implementation is based on the HID report structure  that has been lerned from the device via USBlyzer since the  HID-Report 
provided by the interlink device is incomplete and doesn't support the digital trim switches. The RED LED in the USB stick illuminates when the interlink red reset button is pressed.

The transmitter reads the USB stream and sends out all the stick and switch positions on the RF link. No handscahking with the RX device is done and no frequency hopping is done.

The Receiver (8051) gets the stick positions and adds/subtracts digital trim values from the four main cahnnels then creates the PWM signal train on it's output pin. If no signal is received the roll, pitch, yaw positions go to neutral while throttle goes to 0. 
. 

