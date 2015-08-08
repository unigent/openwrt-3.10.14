/*****************************************************************************
* $File:   watchdog.c
*
* $Author: Hua Shao
* $Date:   Feb, 2014
*
* The dog needs feeding.......
*
*****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/watchdog.h>

static int _running = 1;
void sigterm_handler(int arg)
{
    _running = 0;
}

#if 0
#define TRACE(str) fprintf(stderr, "%s(), L%d. %s\n", __FUNCTION__, __LINE__, str)
#else
#define TRACE(str)
#endif


int main(int argc, char *const argv[])
{
    pid_t pid = 0;
    int fd = 0;
    int opt = 0;
    int ret = 0;

    TRACE("");

    pid = fork();
    if (pid < 0)
    {
        TRACE("fork fail!");
        fprintf(stderr, "fork fail! %s!\n", strerror(errno));
        return -1;
    }
    if (pid>0)
    {
        fprintf(stderr, "watchdog fork, parent exit!\n");
        exit(0);
    }

    /* avoid reseting syste before fully inited. */
    TRACE("sleep");
    sleep(10);
    TRACE("wake watchdog up");

    /* open the device */
    fd = open("/dev/watchdog", O_WRONLY);
    if ( fd == -1 )
    {
        fprintf(stderr, "open /dev/watchdog fail! %s!\n", strerror(errno));
        exit(1);
    }

    TRACE("");
    /* set signal term to call sigterm_handler() */
    /* to make sure fd device is closed */
    signal(SIGTERM, sigterm_handler);

    /* main loop: feeds the dog every <tint> seconds */
    while(_running)
    {
        if(write(fd, "\0", 1)<0)
        {
            TRACE("");
        }
        TRACE("");
        sleep(1);
    }

    fprintf(stderr, "wdt app will quit, sending WDIOS_DISABLECARD to driver. \n");
    opt = WDIOS_DISABLECARD;

__retry:
#if 0
    ret = ioctl(fd, WDIOC_SETOPTIONS, &opt);
    if ( ret == EINTR )
        goto __retry;
    else if (ret < 0)
        fprintf(stderr, "ioctl WDIOS_DISABLECARD fail: %s.\n", strerror(errno));
    else
        fprintf(stderr, "ioctl WDIOS_DISABLECARD done!\n");
#endif
    fprintf(stderr, "app stop feeding.\n");
    if (close(fd) == -1)
    {
        TRACE("");
    }
}


