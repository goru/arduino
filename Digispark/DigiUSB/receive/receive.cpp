#include <stdio.h>

#if defined LINUX
#include <libusb-1.0/libusb.h>
#elif defined DARWIN
#include <libusb.h>
#endif

#include <unistd.h>
#include <signal.h>

int idVendor = 0x16c0;
int idProduct = 0x05df;

libusb_device_handle* findDigiSpark();

int interrupted = 0;
void interruptHandler(int signal) {
    interrupted = 1;
}

int main (int argc, char **argv) {
    libusb_context* context = NULL;

    int r = libusb_init(&context);
    if (r < LIBUSB_SUCCESS) {
        fprintf(stderr, "libusb_init failure: %d\n", r);
        return 1;
    }

    //libusb_set_debug(context, LIBUSB_LOG_LEVEL_DEBUG);

    libusb_device_handle* handle = findDigiSpark();
    if (handle != NULL && signal(SIGINT, interruptHandler) != SIG_ERR) {
        printf("press Ctrl+C to exit\n");

        while (interrupted != 1) {
            unsigned char c;
            int r = libusb_control_transfer(handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE, 0x01, 0, 0, &c, 1, 1000);
            if (r < LIBUSB_SUCCESS) {
                fprintf(stderr, "libusb_control_transfer failure: %d\n", r);
                break;
            }

            if (r > 0) {
                printf("%c", c);
            }

            usleep(100000); // 0.1sec
        }
        
        libusb_close(handle);
    }

    libusb_exit(context);

    printf("exit\n");
}

libusb_device_handle* findDigiSpark() {
    libusb_device** deviceList = NULL;
    int count = libusb_get_device_list(NULL, &deviceList);
    if (count < LIBUSB_SUCCESS) {
        fprintf(stderr, "libusb_get_device_list failure: %d\n", count);
        return NULL;
    }

    libusb_device_handle* handle = NULL;
    for (int i = 0; i < count; i++) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(deviceList[i], &desc);
        if (r < LIBUSB_SUCCESS) {
            fprintf(stderr, "libusb_get_device_descriptor failure: %d\n", r);
            break;
        }

        if (desc.idVendor == idVendor && desc.idProduct == idProduct) {
            printf("Digispark found\n");
            handle = libusb_open_device_with_vid_pid(NULL, desc.idVendor, desc.idProduct);
            if (handle == NULL) {
                fprintf(stderr, "libusb_open_device_with_vid_pid failure\n");
            }
            break;
        }
    }

    if (handle == NULL) {
        fprintf(stderr, "could not find Digispark\n");
    }

    libusb_free_device_list(deviceList, 1);

    return handle;
}

