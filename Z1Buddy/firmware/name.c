// usb_names.h  (put in the same folder as your .ino)
// Customize the USB Manufacturer/Product strings shown by the host.



#include <usb_names.h> // provided by Teensy core

// Change these to your branding
#define MANUFACTURER_NAME  {'S','u','n','d','e','l','i','n','I','n','s','t','r','u','m','e','n','t','s'}
#define MANUFACTURER_NAME_LEN  19

#define PRODUCT_NAME       {'Z','1',' ','B','u','d','d','y'}
#define PRODUCT_NAME_LEN   8

// Optional: rename MIDI interface(s)

struct usb_string_descriptor_struct usb_string_manufacturer_name = {
  2 + MANUFACTURER_NAME_LEN * 2,
  3,
  MANUFACTURER_NAME
};



struct usb_string_descriptor_struct usb_string_product_name = {
  2 + PRODUCT_NAME_LEN * 2,
  3,
  PRODUCT_NAME
};


