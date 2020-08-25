/**************************
 * USB Temperature Sensor *
 * ubld Electronics, LLC. *
 * Chris K Cockrum        *
 * 08/07/2020             *
 * STM32F103C8T6          *
 * 72MHz Clock            *
 * Temp = 1 reading / sec *
 * LED = 8 bit truecolor  *
 *************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include "tempsensor.h"

// These are about 200nS per write 
#define ws2812_zero	gpio_set(GPIOA,GPIO7);gpio_clear(GPIOA,GPIO7);gpio_clear(GPIOA,GPIO7);gpio_clear(GPIOA,GPIO7);gpio_clear(GPIOA,GPIO7)
#define ws2812_one	gpio_set(GPIOA,GPIO7);gpio_set(GPIOA,GPIO7);gpio_set(GPIOA,GPIO7);gpio_set(GPIOA,GPIO7);gpio_clear(GPIOA,GPIO7)

#define i2c_port 	GPIOB
#define i2c_clock 	GPIO6
#define i2c_data 	GPIO7

#define delay(cycles)		for (int i = 0; i < cycles; i++) __asm__("nop")

#define LED_BRIGHTNESS	2 	/* How many bits to shift the RGB values before sending to WS2812 */

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

void ws2812_write(unsigned char red, unsigned char green, unsigned char blue);
void ws2812_write_8bit_truecolor(unsigned char color);

static enum usbd_request_return_codes usb_cdc_handle_control(usbd_device *usbd_handle, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_handle, struct usb_setup_data *req))
{
	char local_buf[10];
	struct usb_cdc_notification *notif = (void *)local_buf;

	switch (req->bRequest) 
	{
		case USB_CDC_REQ_SET_CONTROL_LINE_STATE: 
		{
			notif->bmRequestType = 0xA1;
			notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
			notif->wValue = 0;
			notif->wIndex = 0;
			notif->wLength = 2;
			local_buf[8] = req->wValue & 3;
			local_buf[9] = 0;
			return USBD_REQ_HANDLED;
		}
		case USB_CDC_REQ_SET_LINE_CODING:
		{
			if (*len < sizeof(struct usb_cdc_line_coding))
				return USBD_REQ_NOTSUPP;
			return USBD_REQ_HANDLED;
		}
	}

	return USBD_REQ_NOTSUPP;
}
 
static void usb_cdc_receive(usbd_device *usbd_handle, uint8_t ep)
{
	char buf[64];
	int len = usbd_ep_read_packet(usbd_handle, 0x01, buf, 1);

	
	if (len) {
		ws2812_write_8bit_truecolor(buf[0]);
	}
}

static void usb_cdc_config_setup(usbd_device *usbd_handle, uint16_t wValue)
{
	usbd_ep_setup(usbd_handle, 0x01, USB_ENDPOINT_ATTR_BULK, 64, usb_cdc_receive);
	usbd_ep_setup(usbd_handle, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_handle, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(usbd_handle,USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE, \
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,	usb_cdc_handle_control);
}

void gpio_setup(void) {

	/* Enable GPIO clocks */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	/* For ws2812 */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO7);

	
	/* For i2c clock */
	gpio_set_mode(i2c_port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,	i2c_clock);
	
	
	/* For i2c data */
	gpio_set_mode(i2c_port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, i2c_data);
}

void ws2812_write(unsigned char red, unsigned char green, unsigned char blue)
{
	int i;
	unsigned int temp;

	gpio_clear(GPIOA,GPIO7);

	for (i = 0; i < 500000; i++)	/* Wait a bit. */
		__asm__("nop");

	temp=green;
	for(i=0;i<8;i++)
	{
		if(temp & 0x80)
		{
			ws2812_one;
		}
		else
		{
			ws2812_zero;
		}
		temp<<=1;
	}
	temp=red;
	for(i=0;i<8;i++)
	{
		if(temp & 0x80)
		{
			ws2812_one;
		}
		else
		{
			ws2812_zero;
		}
		
		temp<<=1;
	}
	temp=blue;
	for(i=0;i<8;i++)
	{
		if(temp & 0x80)
		{
			ws2812_one;
		}
		else
		{
			ws2812_zero;
		}
		
		temp<<=1;
	}
	
}

void ws2812_write_8bit_truecolor(unsigned char color)
{
	int i;
	unsigned int temp;

	gpio_clear(GPIOA,GPIO7);

	for (i = 0; i < 500000; i++)	/* Wait a bit. */
		__asm__("nop");

	temp=(color>>2)&0x7;
	temp<<= LED_BRIGHTNESS;
	for(i=0;i<8;i++)
	{
		if(temp & 0x80)
		{
			ws2812_one;
		}
		else
		{
			ws2812_zero;
		}
		temp<<=1;
	}
	temp=(color>>5)&0x7;
	temp<<= LED_BRIGHTNESS;
	for(i=0;i<8;i++)
	{
		if(temp & 0x80)
		{
			ws2812_one;
		}
		else
		{
			ws2812_zero;
		}
		
		temp<<=1;
	}
	temp=color & 0x3;
	temp<<= (LED_BRIGHTNESS+1);
	for(i=0;i<8;i++)
	{
		if(temp & 0x80)
		{
			ws2812_one;
		}
		else
		{
			ws2812_zero;
		}
		
		temp<<=1;
	}
	
}

int read_temperature()
{
	int temp=0;
	int n=0;

	/* Set Data to Output */
	gpio_set_mode(	i2c_port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,	i2c_data);	delay(30);
	
	/* Start Condition */
	gpio_set(i2c_port,i2c_clock);	delay(30);
	gpio_set(i2c_port,i2c_data);	delay(30);
	gpio_clear(i2c_port,i2c_data);	delay(30);
	gpio_clear(i2c_port,i2c_clock);	delay(30);
	
	/* Device Address = 0x48 */
	/* Send Address */
	// 1
	gpio_set(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);

	// 0
	gpio_clear(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);

	// 0
	gpio_clear(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);

	// 1
	gpio_set(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);

	// 0
	gpio_clear(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);

	// 0
	gpio_clear(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);

	// 0
	gpio_clear(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);

	// 1 = Read
	gpio_set(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);

	/* Set data to input with pullup */
	gpio_set_mode(	i2c_port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,	i2c_data);	delay(30);

	/* Clock in acknowledge (ignore it ) */
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);


	temp=0;

	/* Get First 8 Bits */
	for (n=0;n<8;n++)
	{
		temp<<=1;
		gpio_set(i2c_port,i2c_clock);	delay(30);	
		temp |= (gpio_get(i2c_port, i2c_data) != 0);
		gpio_clear(i2c_port,i2c_clock);	delay(30);
	}
	
	/* Send Acknowledge */
	/* Set Data to Output */
	gpio_set_mode(	i2c_port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,	i2c_data);	delay(30);
	gpio_clear(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);
	gpio_set(i2c_port,i2c_data);	delay(30);

	/* Set data to input with pullup */
	gpio_set_mode(	i2c_port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,	i2c_data);delay(30);

	/* Get Last 8 Bits */
	for (n=0;n<8;n++)
	{
		temp<<=1;
		gpio_set(i2c_port,i2c_clock);	delay(30);	
		temp |= (gpio_get(i2c_port, i2c_data) != 0);
		gpio_clear(i2c_port,i2c_clock);	delay(30);
	}

	/* Set Data to Output */
	gpio_set_mode(	i2c_port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,	i2c_data);	delay(30);
	gpio_clear(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);	gpio_clear(i2c_port,i2c_clock);	delay(30);

	
	/* Stop Condition */
	gpio_clear(i2c_port,i2c_data);	delay(30);
	gpio_set(i2c_port,i2c_clock);	delay(30);
	gpio_set(i2c_port,i2c_data);	delay(30);

	/* Return 11 bits */
	return temp>>5;
}

int main(void) 
{
	int i;
	int count=0;
	int tempint;
	float tempvalue=0;

	char buffer[64];

	usbd_device *usbd_handle;

	rcc_clock_setup_in_hse_8mhz_out_72mhz();	// Use this for "blue pill"
	gpio_setup();


	usbd_handle = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_handle, usb_cdc_config_setup);

		for (i = 0; i < 1500000; i++)	
		__asm__("nop");
		ws2812_write(0x10,0x00,0x00);

		for (i = 0; i < 1500000; i++)	
			__asm__("nop");
		ws2812_write(0x10,0x10,0x10);

		for (i = 0; i < 1500000; i++)	
			__asm__("nop");
		ws2812_write(0x00,0x00,0x10);

    /* The first temperature read isn't valid */
	tempint = read_temperature();

	while(1)
	{   

		for (i = 0; i < 1000; i++)	
			__asm__("nop");

		if(count++>10000)
		{
			tempint = read_temperature();

			/* Test Negative */
			//tempint = 0x00000ff8;  // Should be -1.000 C

			/* If Negative then sign extend */
			if(tempint & 0x800)
				tempint = tempint | 0xfffff000;

			/* Convert to float */
			tempvalue = tempint *.125;

			/* Print to a string */
			sprintf(buffer,"%.03f C\n", (double) tempvalue);

			/* Write buffer to USB */
			usbd_ep_write_packet(usbd_handle, 0x82, buffer, strlen(buffer));
			
			count=0;
		}

		usbd_poll(usbd_handle);
	}

	return 0;
}
