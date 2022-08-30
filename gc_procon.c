//
// Created by Shawn Jin on 30/8/2022.
// Inspired by https://gist.github.com/mzyy94/60ae253a45e2759451789a117c59acf9#file-add_procon_gadget-sh
//

#include "gc_procon.h"

static usbg_state *state = NULL;
static char report_desc[] = {0x05, 0x01, 0x15, 0x00, 0x09, 0x04, 0xa1, 0x01, 0x85, 0x30, 0x05, 0x01, 0x05, 0x09, 0x19,
                             0x01, 0x29, 0x0a, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0a, 0x55, 0x00, 0x65, 0x00,
                             0x81, 0x02, 0x05, 0x09, 0x19, 0x0b, 0x29, 0x0e, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95,
                             0x04, 0x81, 0x02, 0x75, 0x01, 0x95, 0x02, 0x81, 0x03, 0x0b, 0x01, 0x00, 0x01, 0x00, 0xa1,
                             0x00, 0x0b, 0x30, 0x00, 0x01, 0x00, 0x0b, 0x31, 0x00, 0x01, 0x00, 0x0b, 0x32, 0x00, 0x01,
                             0x00, 0x0b, 0x35, 0x00, 0x01, 0x00, 0x15, 0x00, 0x27, 0xff, 0xff, 0x00, 0x00, 0x75, 0x10,
                             0x95, 0x04, 0x81, 0x02, 0xc0, 0x0b, 0x39, 0x00, 0x01, 0x00, 0x15, 0x00, 0x25, 0x07, 0x35,
                             0x00, 0x46, 0x3b, 0x01, 0x65, 0x14, 0x75, 0x04, 0x95, 0x01, 0x81, 0x02, 0x05, 0x09, 0x19,
                             0x0f, 0x29, 0x12, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x04, 0x81, 0x02, 0x75, 0x08,
                             0x95, 0x34, 0x81, 0x03, 0x06, 0x00, 0xff, 0x85, 0x21, 0x09, 0x01, 0x75, 0x08, 0x95, 0x3f,
                             0x81, 0x03, 0x85, 0x81, 0x09, 0x02, 0x75, 0x08, 0x95, 0x3f, 0x81, 0x03, 0x85, 0x01, 0x09,
                             0x03, 0x75, 0x08, 0x95, 0x3f, 0x91, 0x83, 0x85, 0x10, 0x09, 0x04, 0x75, 0x08, 0x95, 0x3f,
                             0x91, 0x83, 0x85, 0x80, 0x09, 0x05, 0x75, 0x08, 0x95, 0x3f, 0x91, 0x83, 0x85, 0x82, 0x09,
                             0x06, 0x75, 0x08, 0x95, 0x3f, 0x91, 0x83, 0xc0};

usbg_config *gc_procon_config(usbg_gadget *gadget)
{
    if(gadget == NULL)
        return NULL;

    struct usbg_config_strs config_strs = {
            .configuration = "Nintendo Switch Pro Controller"
    };

    struct usbg_config_attrs config_attrs = {
            .bmAttributes = 160,
            .bMaxPower = 250
    };

    usbg_config *result = usbg_get_first_config(gadget);

    if(result == NULL){
        int usbg_ret = usbg_create_config(gadget,1,"c1",&config_attrs,&config_strs,&result);
        if(usbg_ret != USBG_SUCCESS){
            fprintf(stderr,"failed to get config! \n");
            gc_clean();
            return NULL;
        }
    }

    return result;
}

usbg_gadget * gc_procon_init(gc_generic_info info)
{
    usbg_gadget *g;
    int usbg_ret;

    struct usbg_gadget_attrs g_attrs = {
            .bcdUSB = 0x0200,
            .bDeviceClass =	USB_CLASS_PER_INTERFACE,
            .bDeviceSubClass = 0x00,
            .bDeviceProtocol = 0x00,
            .bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
            .idVendor = info.id_vendor,
            .idProduct = info.id_product,
            .bcdDevice = 0x0200, /* Verson of device */
    };

    struct usbg_gadget_strs g_strs = {
            .serial = info.serial_number, /* Serial number */
            .manufacturer = info.manufacturer, /* Manufacturer */
            .product = info.product /* Product string */
    };

    usbg_ret = usbg_init(GC_CONFIG_PATH, &state);
    if(usbg_ret != USBG_SUCCESS){
        fprintf(stderr,"failed to init gadget. \n");
        state = NULL;
        return NULL;
    }

    /* Another gadget named procon */
    g = usbg_get_gadget(state, "procon");
    if(g == NULL){
        usbg_ret = usbg_create_gadget(state, "procon", &g_attrs, &g_strs, &g);
        if (usbg_ret != USBG_SUCCESS) {
            fprintf(stderr,"failed to create gadget. \n");
            state = NULL;
            return NULL;
        }
    }

    /* disable gadget for now to apply new config */
    usbg_disable_gadget(g);

    return g;
}

int gc_procon_create(int argc, char *argv[], gc_generic_info info)
{
    usbg_gadget *gadget = gc_procon_init(info);

    if(gadget == NULL)
        return GC_FAILED;

    usbg_function *f_hid;
    int usbg_ret;

    char *id = gc_generate_id(USBG_F_HID);

    if(id == NULL)
        return GC_FAILED;

    struct usbg_f_hid_attrs f_attrs = {
            .protocol = 0,
            .report_desc = {
                    .desc = report_desc,
                    .len = sizeof(report_desc),
            },
            .report_length = 64,
            .subclass = 0,
    };

    usbg_ret = usbg_create_function(gadget,USBG_F_HID,id,&f_attrs,&f_hid);
    if(usbg_ret != USBG_SUCCESS){
        fprintf(stderr,"failed to create hid function! (maybe kernel module not loaded?)\n");
        gc_clean();
        return GC_FAILED;
    }

    usbg_config *config = gc_procon_config(gadget);

    usbg_ret = usbg_add_config_function(config,id,f_hid);
    if(usbg_ret != USBG_SUCCESS){
        fprintf(stderr,"failed to bind hid config to function! \n");
        gc_clean();
        return GC_FAILED;
    }

    gc_clean();
    return GC_SUCCESS;
}