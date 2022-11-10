/*
 * main.c
 *
 *  Created on: 10-Oct-2022
 *      Author: user
 */
#include "dbus-client.h"
#include <stdio.h>
#include "logmodule.h"
#include <unistd.h>
#include <string.h>

#define DEBUS_SIGNAL_TOPIC1 "dbus_topic1"
#define DEBUS_METHOD_1 "dbus_method_1"

/**
 * this example application work with example application app3.
 * app4 publish DEBUS_SIGNAL_TOPIC1 signal with app1data_t type data.
 */


typedef struct app1data
{
    int age;
    char name[10];
}app1data_t;

int main(int argc, char **argv) {
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    char *appname=&(argv[0][2]);
	dbus_client_init(appname);
	char signal_payload[32]={0};
	int i=0;
    app1data_t app1data ={23,"thejas"};
	while (1)
	{
		sleep(4);
        app1data.age+=1;
		dbus_client_publish(DEBUS_SIGNAL_TOPIC1, (void *)&app1data, sizeof(app1data));
	}
}


