#include <stdio.h>

#if defined LINUX
#include <libusb-1.0/libusb.h>
#elif defined DARWIN || defined CYGWIN
#include <libusb.h>
#endif

#include <string.h>

int ID_VENDOR  = 0x16c0;
int ID_PRODUCT = 0x05df;
int CMD_WRITE  = 0x0009;

libusb_device_handle* findDigispark();

int main (int argc, char **argv) {
    int ret = 0;

    if (argc != 2) {
        fprintf(stderr, "missing argument\n");
        return 1;
    }

    libusb_context* context = NULL;
    ret = libusb_init(&context);
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "libusb_init failure: %d\n", ret);
        return ret;
    }

#ifdef DEBUG
    libusb_set_debug(context, LIBUSB_LOG_LEVEL_DEBUG);
#endif

    libusb_device_handle* handle = findDigispark();
    if (handle != NULL) {
        int i;
        for (i = 0; i < strlen(argv[1]); i++) {
            ret = libusb_control_transfer(handle,
                LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE,
                CMD_WRITE, 0, argv[1][i], 0, 0, 1000);
            if (ret < LIBUSB_SUCCESS) {
                fprintf(stderr, "libusb_control_transfer failure: %d\n", ret);
                break;
            }
        }

        ret = libusb_control_transfer(handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE,
            CMD_WRITE, 0, '\n', 0, 0, 1000);
        if (ret < LIBUSB_SUCCESS) {
            fprintf(stderr, "libusb_control_transfer failure: %d\n", ret);
        }

        libusb_close(handle);
    }

    libusb_exit(context);

    return ret;
}

libusb_device_handle* findDigispark() {
    libusb_device** deviceList = NULL;
    int count = libusb_get_device_list(NULL, &deviceList);
    if (count < LIBUSB_SUCCESS) {
        fprintf(stderr, "libusb_get_device_list failure: %d\n", count);
        return NULL;
    }

    libusb_device_handle* handle = NULL;

    int i;
    for (i = 0; i < count; i++) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(deviceList[i], &desc);
        if (r < LIBUSB_SUCCESS) {
            fprintf(stderr, "libusb_get_device_descriptor failure: %d\n", r);
            break;
        }

        if (desc.idVendor == ID_VENDOR && desc.idProduct == ID_PRODUCT) {
            printf("Digispark found\n");
            handle = libusb_open_device_with_vid_pid(NULL, desc.idVendor, desc.idProduct);
            if (handle == NULL) {
                fprintf(stderr, "libusb_open_device_with_vid_pid failure\n");
            }
            break;
        }
    }

    if (handle == NULL) {
        fprintf(stderr, "could not find/open Digispark\n");
    }

    libusb_free_device_list(deviceList, 1);

    return handle;
}

