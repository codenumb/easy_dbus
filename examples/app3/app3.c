#include <stdio.h>
#include "logmodule.h"
#include "dbus-client.h"
#include <unistd.h>
#include <string.h>

#define DEBUS_SIGNAL_TOPIC1 "dbus_topic1"
#define DEBUS_METHOD_1 "dbus_method_1"
#define DBUS_REMOTE_APP_BUS_NAME "app1"

/**
 * this example application work with example application app4.
 * app3 is subscribed to th DEBUS_SIGNAL_TOPIC1 signal emited by app4.
 * handle_dbus_signal_topic_1 is the handler function for the signal and it will simply print the data.
 */


typedef struct app1data
{
    int age;
    char name[10];
}app1data_t;

void handle_dbus_signal_topic_1(char *value, int len)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    app1data_t *app1data = (app1data_t *)(value);

    printf("%s:%d name=%s age=%d\n",__func__,__LINE__,app1data->name,app1data->age);

    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}

int main(int argc, char **argv) {
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    char *appname=&(argv[0][2]);
    dbus_client_init(appname);
    dbus_client_subscribe (1,
    						DEBUS_SIGNAL_TOPIC1, (void *)handle_dbus_signal_topic_1
    					   );
    while(1)
    {
    	sleep(2);
    }
}
