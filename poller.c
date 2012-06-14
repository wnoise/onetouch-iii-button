#include <errno.h>
#include <fcntl.h>
#include <stdint.h>     
#include <stdio.h>     
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>    
#include <unistd.h> 

#include <linux/input.h>
#include <linux/uinput.h>

#include <scsi/sg_lib.h>
#include <scsi/sg_cmds_basic.h>

static const int supported_vpds []  = { 0x00, 0x80, 0xc0, 0xc1, 0xc2, 0xc3 };
static const int constant_status_vpd_bytes []  =  { 0x01, 0x00, 0x00, 0x00 };
static const int vpd_header_size = 4;
static const uint16_t vendor = 0xd49;
static const uint16_t product = 0x7201;
static const uint8_t status_vpd_num = 0xc3;
static const int supported_vpds_count = 6;
static const int status_vpd_count = 6;
static const int constant_status_vpd_count = 4;
static const int since_index = 4;
static const int now_index = 5;

static void daemonize(void)
{
    pid_t child = fork();
    switch (child) {
        case -1: /* Failed */
            puts("Can't fork()");
            exit(EXIT_FAILURE);
        case 0: /* child */
            break;
        default: /* parent */
            exit(EXIT_SUCCESS);
            break;
    }
}

static void usage()
{
    fputs("Usage: onetouch-iii-daemon <device>\n", stderr);
    exit(EXIT_FAILURE);
}

static void parse(int argc, char **argv, char **scsiname)
{
    if (argc != 2) {
        usage();
    }
    if (argv[1][0] == '-') {
        usage();
    }
    *scsiname = argv[1];
}

static void open_files(char *scsiname, int *scsi_fd, int *uinput_fd)
{
        int fd;
        errno = 0;
        fd = open(scsiname, O_RDONLY);
        if (fd < 0) {
            fputs("open ", stderr);
            perror(scsiname);
            exit(EXIT_FAILURE);
        }
        *scsi_fd = fd;
        fd  = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (fd < 0) {
            perror("open /dev/uinput");
            exit(EXIT_FAILURE);
        }
        *uinput_fd = fd;
}

static void drop_privs()
{
    int rv = setuid(65534);
    if (!getuid() || !geteuid()) {
        exit(EXIT_FAILURE);
    }
}

static int vpd_header_okay(uint8_t vpd_num, uint8_t len, uint8_t *header)
{
     return header[0] == 0 && header[1] == vpd_num && header[2] == 0 && header[3] == len;
}

static void check_vpd_list(int scsi_fd)
{
    int res;
    char buffer[32];
    res = sg_ll_inquiry(scsi_fd, 0, 1, 0, buffer, 32, 0, 0);
    if (res) {
        fputs("Can't read list of supported VPDs\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (!vpd_header_okay(0, supported_vpds_count, buffer)) {
        fputs("Unexpected list of supported VPDs\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (memcmp(buffer + vpd_header_size, supported_vpds,
                supported_vpds_count)) {
        fputs("Unexpected list of supported VPDs\n", stderr);
        exit(EXIT_FAILURE);
    }
}

static void check_status_vpd(int scsi_fd)
{
    int res;
    char buffer[32];
    res = sg_ll_inquiry(scsi_fd, 0, 1, status_vpd_num, buffer, 32, 0, 0);
    if (res) {
        fputs("Can't read status VPD\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (!vpd_header_okay(status_vpd_num, 6, buffer)) {
        fputs("Unexpected status VPD format\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (memcmp(buffer + vpd_header_size, constant_status_vpd_bytes,
                constant_status_vpd_count)) {
        fputs("Unexpected status VPD format\n", stderr);
        exit(EXIT_FAILURE);
    }
}

static void setup_uinput(int uinput_fd)
{
       struct uinput_user_dev uinp = {
           .name = "OneTouch III Button",
           .id.vendor = vendor,
           .id.product = product,
           .id.bustype = BUS_USB
       };
       ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
       ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN);
       ioctl(uinput_fd, UI_SET_KEYBIT, KEY_PROG1);
       write(uinput_fd, &uinp, sizeof uinp);
       ioctl(uinput_fd, UI_DEV_CREATE);
}

static void poll_scsi(int scsi_fd, int *since_last, int *now)
{
    int res;
    uint8_t buffer[32];
    res = sg_ll_inquiry(scsi_fd, 0, 1, status_vpd_num, buffer, 32, 0, 0);
    if (res) {
        fputs("Can't read status VPD\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (!vpd_header_okay(status_vpd_num, 6, buffer)) {
        fputs("Unexpected status VPD format\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (memcmp(buffer + vpd_header_size, constant_status_vpd_bytes,
                constant_status_vpd_count)) {
        fputs("Unexpected status VPD format\n", stderr);
        exit(EXIT_FAILURE);
    }
    *since_last = buffer[vpd_header_size + since_index];
    *now = buffer[vpd_header_size + now_index];
}

static void send_uinput_event(int uinput_fd, int since_last, int now)
{
    struct input_event
        press_event = { .type = EV_KEY, .code = KEY_PROG1, .value = 1},
        release_event = { .type = EV_KEY, .code = KEY_PROG1, .value = 0},
        syn_event = { .type = EV_SYN, .code = SYN_REPORT, .value = 0};
    if (since_last) {
        gettimeofday(&press_event.time, NULL);
        write(uinput_fd, &press_event, sizeof press_event);
        gettimeofday(&syn_event.time, NULL);
        write(uinput_fd, &syn_event, sizeof syn_event);
        gettimeofday(&release_event.time, NULL);
        write(uinput_fd, &release_event, sizeof release_event);
        gettimeofday(&syn_event.time, NULL);
        write(uinput_fd, &syn_event, sizeof syn_event);
    }
}

static void mainloop(int scsi_fd, int uinput_fd)
{
    int since_last, now;
    poll_scsi(scsi_fd, &since_last, &now);
    for (;;) {
        sleep(1);
        poll_scsi(scsi_fd, &since_last, &now);
        send_uinput_event(uinput_fd, since_last, now);
    }
}

int main(int argc, char **argv)
{
        char *scsiname;
        int scsi_fd, uinput_fd;
        parse(argc, argv, &scsiname);
        open_files(scsiname, &scsi_fd, &uinput_fd);
        drop_privs();
        check_vpd_list(scsi_fd);
        check_status_vpd(scsi_fd);
        setup_uinput(uinput_fd);
        daemonize();
        mainloop(scsi_fd, uinput_fd);
        return 0;
}
