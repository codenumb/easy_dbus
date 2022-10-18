#include <stdio.h>
#include "logmodule.h"
#include "dbus-client.h"
#include <unistd.h>
#include <string.h>

#define DEBUS_SIGNAL_TOPIC1 "dbus_topic1"
#define DEBUS_METHOD_1 "dbus_method_1"
#define DBUS_REMOTE_APP_BUS_NAME "app1"

void handle_dbus_signal_topic_1(char *value, int len)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);

    printf("%s:%d %s\n",__func__,__LINE__,value);

    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}

typedef struct example_reply{
	int number;
	char character;
}example_reply_t;



int main(int argc, char **argv) {
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    char *appname=&(argv[0][2]);
    dbus_client_init(appname);
    dbus_client_subscribe (1,
    						DEBUS_SIGNAL_TOPIC1, (void *)handle_dbus_signal_topic_1
    					   );
    example_reply_t *reply={0};
    int replylen=0;
    char *arg="hello from app2";
    int arglen=strlen(arg);
    while(1)
    {
    	sleep(2);
    	if(!dbus_client_call_remote_method_with_reply(DBUS_REMOTE_APP_BUS_NAME, DEBUS_METHOD_1, arg, arglen, (void **)&reply, &replylen))
    	{
    		printf("%s:%d reply.character=%c reply.number=%d\n",__func__,__LINE__,reply->character,reply->number);
    	}
    }
}
