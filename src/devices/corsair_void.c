#include "../device.h"
#include "../utility.h"

#include <hidapi.h>
#include <string.h>


static struct device device_void;

enum void_wireless_battery_flags {
    VOID_BATTERY_MICUP = 128
};

#define ID_CORSAIR_VOID_WIRELESS        0x1b27
#define ID_CORSAIR_VOID_PRO             0x0a14
#define ID_CORSAIR_VOID_PRO_R2          0x0a16
#define ID_CORSAIR_VOID_PRO_WIRELESS    0x0a1a
#define ID_CORSAIR_VOID_RGB_USB         0x1b2a
#define ID_CORSAIR_VOID_RGB_WIRED       0x1b1c

static const uint16_t PRODUCT_IDS[] = {ID_CORSAIR_VOID_RGB_WIRED,ID_CORSAIR_VOID_WIRELESS, ID_CORSAIR_VOID_PRO, ID_CORSAIR_VOID_PRO_R2, ID_CORSAIR_VOID_PRO_WIRELESS, ID_CORSAIR_VOID_RGB_USB};

static int void_send_sidetone(hid_device *device_handle, uint8_t num);
static int void_request_battery(hid_device *device_handle);
static int void_notification_sound(hid_device *hid_device, uint8_t soundid);
static int void_lights(hid_device *device_handle, uint8_t on);

void void_init(struct device** device)
{
    device_void.idVendor = VENDOR_CORSAIR;
    device_void.idProductsSupported = PRODUCT_IDS;
    device_void.numIdProducts = sizeof(PRODUCT_IDS)/sizeof(PRODUCT_IDS[0]);
    
    strcpy(device_void.device_name, "Corsair Void (Pro)");
    
    device_void.capabilities = CAP_SIDETONE | CAP_BATTERY_STATUS | CAP_NOTIFICATION_SOUND | CAP_LIGHTS;
    device_void.send_sidetone = &void_send_sidetone;
    device_void.request_battery = &void_request_battery;
    device_void.notifcation_sound = &void_notification_sound;
    device_void.switch_lights = &void_lights;
    
    *device = &device_void;
}

static int void_send_sidetone(hid_device *device_handle, uint8_t num)
{
    // the range of the void seems to be from 200 to 255
    num = map(num, 0, 128, 200, 255);

    unsigned char data[12] = {0xFF, 0x0B, 0, 0xFF, 0x04, 0x0E, 0xFF, 0x05, 0x01, 0x04, 0x00, num};

    return hid_send_feature_report(device_handle, data, 12);
}

static int void_request_battery(hid_device *device_handle)
{
    // Packet Description
    // Answer of battery status
    // Index    0   1   2       3       4
    // Data     100 0   Loaded% 177     5 when loading, 1 otherwise
    
    
    int r = 0;
    
    // request battery status
    unsigned char data_request[2] = {0xC9, 0x64};
    
    r = hid_write(device_handle, data_request, 2);
    
    if (r < 0) return r;
    
    // read battery status
    unsigned char data_read[5];
    
    r = hid_read(device_handle, data_read, 5);
 
    if (r < 0) return r;
    
    if (data_read[4] == 0 || data_read[4] == 4 || data_read[4] == 5)
    {
        return BATTERY_CHARGING;
    }
    else if (data_read[4] == 1)
    {
        // Discard VOIDPRO_BATTERY_MICUP when it's set
        // see https://github.com/Sapd/HeadsetControl/issues/13
        if (data_read[2] & VOID_BATTERY_MICUP)
        {
            return data_read[2] &~ VOID_BATTERY_MICUP;
        }
        else
        {
            return (int)data_read[2]; // battery status from 0 - 100
        }
    }
    else
    {
        return HSC_ERROR;
    }
}

static int void_notification_sound(hid_device* device_handle, uint8_t soundid)
{
    // soundid can be 0 or 1
    unsigned char data[5] = {0xCA, 0x02, soundid};
    
    return hid_write(device_handle, data, 3);
}

static int void_lights(hid_device* device_handle, uint8_t on)
{
    unsigned char data[3] = {0xC8, on ? 0x00 : 0x01, 0x00};
    return hid_write(device_handle, data, 3);
}
