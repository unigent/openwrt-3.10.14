#include <getopt.h>
#include <string.h>
#include <stdio.h>

#include <signal.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
//#include <sys/types.h>
//#include <linux/time.h>
//#include <linux/if.h>
#include <net/if.h>
#include <asm/types.h>

//#include <linux/mii.h>
//#include <linux/types.h>
#include <linux/autoconf.h>
//#include <config/autoconf.h>
#include "ra_ioctl.h"

#define LOOP_DELAY 500000 // in us


void clear_max_gain_flag(int port_num)
{
	int sk, method, ret, i;
	int address[6] = {31, 16, 31, 26, 31, 16};
	int value[6] = {0x8000, 0x0001, 0x0000, 0x0002, 0x8000, 0x0000};
	struct ifreq ifr;
	ra_mii_ioctl_data mii;

	sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (sk < 0) {
		printf("Open socket failed\n");
	}

	strncpy(ifr.ifr_name, "eth0", 5);
        ifr.ifr_data = &mii;

	for( i=0; i<6; i++){
		method = RAETH_MII_WRITE;
		mii.phy_id = port_num;
		mii.reg_num = address[i];
		mii.val_in =  value[i];
		ret = ioctl(sk, method, &ifr);
	}
	//printf("clear port#%d max gain flag\n", port_num);
	close(sk);
}

int read_max_gain_flag(int port_num)
{
	int sk, method, ret;
	struct ifreq ifr;
	ra_mii_ioctl_data mii;

	sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (sk < 0) {
		printf("Open socket failed\n");
		return -1;
	}

        strncpy(ifr.ifr_name, "eth0", 5);
	ifr.ifr_data = &mii;

	method = RAETH_MII_WRITE;
	mii.phy_id = port_num;
	mii.reg_num = 31;
	mii.val_in =  0x8000;
	ret = ioctl(sk, method, &ifr);

	method = RAETH_MII_READ;
	mii.phy_id = port_num;
	mii.reg_num = 22;
	ret = ioctl(sk, method, &ifr);
        close(sk);

	if(mii.val_out&0x0400)
		return 1;
	else
		return 0;
}


int read_signal_detect_flag(int port_num)
{
	static int Up2Down_cnt[5] = {0, 0, 0, 0, 0};
	int sk, method, ret;
	struct ifreq ifr;
	ra_mii_ioctl_data mii;

	sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (sk < 0) {
		printf("Open socket failed\n");
		return -1;
	}

	strncpy(ifr.ifr_name, "eth0", 5);
	ifr.ifr_data = &mii;

	method = RAETH_MII_WRITE;
	mii.phy_id = port_num;
	mii.reg_num = 31;
	mii.val_in =  0x8000;
	ret = ioctl(sk, method, &ifr);

	method = RAETH_MII_READ;
	mii.phy_id = port_num;
	mii.reg_num = 28;
	ret = ioctl(sk, method, &ifr);
	close(sk);

	if ((mii.val_out & 0x0001) == 0x0001 ){ // link up
		if(Up2Down_cnt[port_num] > 0)
			Up2Down_cnt[port_num]--;
	}else if ((mii.val_out & 0x8000) == 0x8000 ){ //plug but not link up
		Up2Down_cnt[port_num]++;
	}

	if( Up2Down_cnt[port_num] > 3){
		Up2Down_cnt[port_num] = 0;
		return 1;
	}else
		return 0;
}

int read_link_status(int port_num)
{
	int sk, method, ret;
	struct ifreq ifr;
	ra_mii_ioctl_data mii;

	sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (sk < 0) {
		printf("Open socket failed\n");
		return -1;
	}

	strncpy(ifr.ifr_name, "eth0", 5);
	ifr.ifr_data = &mii;

	method = RAETH_MII_WRITE;
	mii.phy_id = port_num;
	mii.reg_num = 31;
	mii.val_in =  0x8000;
	ret = ioctl(sk, method, &ifr);

	method = RAETH_MII_READ;
	mii.phy_id = port_num;
	mii.reg_num = 28;
	ret = ioctl(sk, method, &ifr);
	close(sk);

	if(mii.val_out&0x0001)
		return 1;
	else
		return 0;

}


void set_100M_extend_setting(int port_num)
{

	int sk, method, ret, i;
	struct ifreq ifr;
	ra_mii_ioctl_data mii;
	int address[] = {31, 19, 31, 21, 31, 0};
        int value[] = {0x8000, 0x6710, 0xa000, 0x8513, 0x8000, 0x3100};

	sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (sk < 0) {
		printf("Open socket failed\n");
	}
         
	strncpy(ifr.ifr_name, "eth0", 5);
        ifr.ifr_data = &mii;

	for( i=0; i<4; i++){
		method = RAETH_MII_WRITE;
		mii.phy_id = port_num;
		mii.reg_num = address[i];
		mii.val_in =  value[i];
		ret = ioctl(sk, method, &ifr);
	}

}

void set_100M_normal_setting(int port_num)
{

	int sk, method, ret, i;
	struct ifreq ifr;
	ra_mii_ioctl_data mii;
	int address[] = {31, 19, 31, 21, 31, 4, 0};
	int value[] = {0x8000, 0x6750, 0xa000, 0x8553, 0x8000, 0x5e1, 0x3100};

	sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (sk < 0) {
		printf("Open socket failed\n");
	}

	strncpy(ifr.ifr_name, "eth0", 5);
	ifr.ifr_data = &mii;

	for( i=0; i<7; i++){
		method = RAETH_MII_WRITE;
		mii.phy_id = port_num;
		mii.reg_num = address[i];
		mii.val_in =  value[i];
		ret = ioctl(sk, method, &ifr);
	}
	close(sk);
}

void set_10M_setting(int port_num)
{
	int sk, method, ret, i;
	struct ifreq ifr;
	ra_mii_ioctl_data mii;

	sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (sk < 0) {
		printf("Open socket failed\n");
	}

	strncpy(ifr.ifr_name, "eth0", 5);
	ifr.ifr_data = &mii;

	method = RAETH_MII_WRITE;
	mii.phy_id = port_num;
	mii.reg_num = 4;
	mii.val_in = 0x0461;
	ret = ioctl(sk, method, &ifr);
        //restart AN

	method = RAETH_MII_WRITE;
	mii.phy_id = port_num;
	mii.reg_num = 0;
	mii.val_in = 0x3100;
	ret = ioctl(sk, method, &ifr);	
	close(sk);
}

static void usage(void)
{
    printf("USAGE :  	longloopd [optional command]\n");
    printf("[optional command] : \n");
    printf("start: start longloopd daemon\n");
    printf("stop : stop longloopd daemon\n");

    exit(1);
}

/******************************************************************************/
/*
 *	Write pid to the pid file
 */
int writePid(pid_t pid)
{
	FILE *fp;

	fp = fopen("/var/run/longloopd.pid", "w+");
	if (NULL == fp) {
		printf("long_loop.c: cannot open pid file");
		return (-1);
	}
	fprintf(fp, "%d", pid);
    fclose(fp);
	return 0;
}

int readPid(void)
{
	int gopid;
	FILE *fp = fopen("/var/run/longloopd.pid", "r");
	
	if (NULL == fp) {
		printf("longloopd: is not running\n");
		return -1;
	}
	fscanf(fp, "%d", &gopid);
	if (gopid < 2) {
		printf("longloopd: pid(%d) <= 1\n", gopid);
		return -1;
	}
	return gopid;
}

int main(int argc, char *argv[])
{
	int ret, i;
	int pre_link_state = 0;
	int port_state[5] = {0, 0, 0, 0, 0}; // 0: 100M normal; 1: 100M extend; 2: 10M
	int port_skip_loop_cnt[5] = {0, 0, 0, 0, 0}; // skip loop count for mode change
	pid_t child_pid;
	pid_t auth_pid;

	printf("program name :'%s'\n", argv[0]);

    for (;;)
    {		
		if(argc == 2)
		{
			if(!strcmp(argv[1],"start"))
			{
				auth_pid = readPid();
				if(auth_pid >= 2)
				{
					printf("longloopd: is running!\n");
					exit(1);
				}
				else
					break;

			}
			else if(!strcmp(argv[1],"stop"))
			{
				auth_pid = readPid();
				if(auth_pid < 2)
				{
					printf("longloopd: is not running!\n");
					exit(1);
				}
				else
				{
					kill(auth_pid, SIGTERM);
					writePid(-1);
					exit(1);
				}	
			}
			else
			{
				usage();
			}	
		}
		else
		{
			 usage();
		}
    }	
	
    child_pid = fork();
    if (child_pid == 0)
    {
    	auth_pid = getpid();
		printf("long loop daemon start,Porcess ID = %d\n",auth_pid);
		writePid(auth_pid);
		for( i = 0; i < 5; i++)	{
			clear_max_gain_flag(i);
		}

		while(1){
			for (i=0; i<5;i++){
				if(port_skip_loop_cnt[i] > 0){
					port_skip_loop_cnt[i]--;
				}else{
					if(port_state[i]==0){ //at 100M normal setting
#ifdef CONFIG_USER_LONG_LOOP_3_STATE
						if(read_max_gain_flag(i)){			
							port_state[i] = 1;
							clear_max_gain_flag(i);
							set_100M_extend_setting(i);
							port_skip_loop_cnt[i] = 6;
							printf("port#%d switch to 100M extend setting\n",i);
						}else 
#endif						
							if(read_signal_detect_flag(i)){
								port_state[i]=2;
								set_10M_setting(i);
								port_skip_loop_cnt[i] = 10;
								printf("port#%d switch to 10M setting\n",i);
							}
					}else if(port_state[i]==1){// at 100M extend setting
						if(!read_link_status(i)){
							if(pre_link_state == 1){//unplug calbe
								port_state[i]=0;
								clear_max_gain_flag(i);
								set_100M_normal_setting(i);
								printf("port#%d switch to 100M normal setting\n",i);
							}else /*if(read_max_gain_flag(i))*/{
								port_state[i]=2;
								set_10M_setting(i);
								port_skip_loop_cnt[i] = 10;
								printf("port#%d switch to 10M setting\n",i);
							}
							pre_link_state = 0;
						}else{
							pre_link_state = 1;
						}
					}else if(port_state[i]==2){   //at 10M mode
						if(!read_link_status(i)){
							port_state[i]=0;
							clear_max_gain_flag(i);
							set_100M_normal_setting(i);
							printf("port#%d switch to 100M normal setting\n",i);
						}
					}
				}
			}
			usleep(LOOP_DELAY);

		}
    }
	
	else
		return 0;

}


