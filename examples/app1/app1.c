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

typedef struct example_reply{
	int number;
	char character;
}example_reply_t;

method_reply_t *handle_dbus_method_1_call(void *data,int size)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);

    method_reply_t *reply_buff;
    char *value=(char*)data;
    printf("%s:%d: argument received= %s\n",__func__,__LINE__,value);
    example_reply_t reply={5,'a'};
    int len = sizeof(reply);
    reply_buff=dbus_client_init_reply_buff(len);
    memcpy((method_reply_t *)reply_buff->data,&reply,len);
    printf("%s:%d: return size= %d data = %c,%d \n",__func__,__LINE__,len,reply.character,reply.number);

    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
    return reply_buff;
}

int main(int argc, char **argv) {
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    char *appname=&(argv[0][2]);
	dbus_client_init(appname);
	char signal_payload[32]={0};
	int i=0;
    dbus_client_subscribe (1,
    					DEBUS_METHOD_1,(void *)handle_dbus_method_1_call
    					   );
	while (1)
	{
		sleep(10);
		sprintf(signal_payload,"topic1_message %d",++i);
		dbus_client_publish(DEBUS_SIGNAL_TOPIC1, (void *)signal_payload, strlen(signal_payload));
	}
}


