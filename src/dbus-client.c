#define LOG_TAG "DBUS"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dbus/dbus.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <dbus-1.0/dbus/dbus.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <event2/event.h>
#include <event2/event_struct.h>

//#include "btrkr-log-util.h"
#include "logmodule.h"
#include "dbus-client.h"

#define DBUS_OBJECT_NAME "/open/dbus/object"
#define DBUS_INTERFACE_NAME "open.dbus.iface"
#define DBUS_REMOTE_APP_BUS_NAME "proc.rxbus.%s"
#define DEBUG printf("%s:%d\n",__func__,__LINE__);

typedef struct
{
    char method[255];
    void *cb;
} method_cb_pair_t;

typedef struct
{
    method_cb_pair_t mthod_cb[32];
    int num_pairs;
} rx_thread_args_t;

typedef struct dbus_ctx
{
    DBusConnection *conn;
    struct event_base *evbase;
    struct event dispatch_ev;
    void *extra;   // watch
    void *extra_t; // timeout
    rx_thread_args_t *args;
} dbus_ctx_t;

static pthread_mutex_t init_dbus_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool init_dbus_flag = false;
static DBusConnection *txconn, *rxconn;
static pthread_t dbus_client_rx_thread;
static char txbusname[255] = "proc.txbus.";
static char rxbusname[255] = "proc.rxbus.";
static struct event_base *ev_base = NULL;
static dbus_ctx_t gctx;

typedef void (*callback)(void *data, size_t len);
typedef method_reply_t* (*method_callback)(void *data, size_t len);

/**
 * Use this function to initialize the dbus method reply buffer.
 * no need to call de-init.
 * @param len required buffer legth in bytes.
 * @return -ve error code on failure and method_reply on success.
 */
method_reply_t *dbus_client_init_reply_buff(int len)
{
	method_reply_t *buff= (method_reply_t*)malloc(sizeof(method_reply_t));
	buff->data= (char *)calloc(len,1);
	buff->len=len;
	return buff;
}
/**
 * Wrapper to call a method  of a remote process.This function is a blocking call.
 * the remote method must respond with a return value.
 * @param appname name of the remote process bus name.
 * @param method remote method to be invoked
 * @param payload argument to be passed to the remote method.
 * @param payloadlen length of the argument
 * @param reply buffer to store the reply from remote method.
 * @return -ve error code on failure and 0 on success.
 */
int  dbus_client_call_remote_method_with_reply(char *appname,char *method,void *payload,
		int payloadlen,void **reply, int *replylen )
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
	DBusError err;
	DBusMessage* msg,*msg_reply;
    DBusMessageIter args,args1;
    DBusMessageIter subiter,subiter1;
	DBusPendingCall* pending;
	int ret=0;
	char busname[64]={0};
	sprintf(busname,DBUS_REMOTE_APP_BUS_NAME,appname);
	char *pload = malloc(payloadlen);
    memcpy(pload, payload, payloadlen);
	dbus_error_init(&err);
  	msg = dbus_message_new_method_call(
  			busname,   	// target for the method call
			DBUS_OBJECT_NAME,  	// object to call on
			DBUS_INTERFACE_NAME, 	// interface to call
            method);//method to call
	if (NULL == msg) {
      fprintf(stderr, "Message Null\n");
      floge("dbustest.log","%s:%d Message Null",__func__,__LINE__);
      ret=-1;
      goto FAIL;
	}
    // append arguments
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "y", &subiter);
    dbus_message_iter_append_fixed_array(&subiter, DBUS_TYPE_BYTE, &pload, payloadlen);
    dbus_message_iter_close_container(&args, &subiter);
	if (!dbus_connection_send_with_reply (txconn, msg, &pending, -1))
	{ // Use -1 for default timeout
		fprintf(stderr, "Out Of Memory!\n");
		floge("dbustest.log","%s:%d Out Of Memory",__func__,__LINE__);
		dbus_message_unref(msg);
		ret=-2;
		goto FAIL;
	}
	if (NULL == pending)
	{
		fprintf(stderr, "Pending Call Null\n");
		floge("dbustest.log","%s:%d Pending Call Null",__func__,__LINE__);
		dbus_message_unref(msg);
		ret=-3;
		goto FAIL;
   	}
	dbus_connection_flush(txconn);
	dbus_message_unref(msg);
	dbus_pending_call_block(pending);
	msg_reply = dbus_pending_call_steal_reply(pending);
   	if (NULL == msg_reply) {
      fprintf(stderr, "Reply Null\n");
      floge("dbustest.log","%s:%d Reply Null",__func__,__LINE__);
      dbus_pending_call_unref(pending);
      ret=-4;
      goto FAIL;
   	}
	dbus_pending_call_unref(pending);
	//	char *buff="test";
	//   	int leng=0;
	//	if (dbus_message_get_args(msg, &err,DBUS_TYPE_STRING, &buff,DBUS_TYPE_INVALID))
	//	 printf("The server answered: '%s'\n", buff);
	if (!dbus_message_iter_init(msg_reply, &args1))
	{
		flogd("dbustest.log", "%s:%d Message has no arguments!",__func__,__LINE__);
	}

	if( dbus_message_get_type (msg_reply) == DBUS_MESSAGE_TYPE_ERROR)
	{
	      floge("dbustest.log","%s:%d Reply is DBUS_MESSAGE_TYPE_ERROR",__func__,__LINE__);
	      *reply=NULL;
	      replylen=0;
	      ret=-5;
	      goto FAIL;
	}
	dbus_message_iter_recurse(&args1, &subiter1);
	dbus_message_iter_get_fixed_array (&subiter1, reply, replylen);
FAIL:
	free(pload);
    dbus_error_free(&err);
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
    return ret;
}

/**
 * internal function, called on dbus remote method call.
 * Note:the user must initialize the method reply buffer using dbus_client_init_reply_buff
 * and fill the buffer with appropriate data and this function will free the method_reply buffer.
 * @param msg dbus message handler.
 * @param conn dbus connection handler.
 * @param cb method to be called.
 */
static void handle_method_call_with_reply (DBusMessage *msg, DBusConnection *conn, void *cb)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    DBusMessageIter args;
    DBusMessageIter subiter;
    DBusError err;
    DBusMessage *reply = NULL;
    method_reply_t *method_reply={NULL};
    method_callback mycb = (method_callback)cb;
    char *value = NULL;
    int len;

    dbus_error_init(&err);
    // read the arguments
    if (!dbus_message_iter_init(msg, &args))
    	flogd("dbustest.log", "%s:%d Message has no arguments!",__func__,__LINE__);

    // TODO: Check these APIs to get the args from the msg.
    dbus_message_iter_recurse(&args, &subiter);
    dbus_message_iter_get_fixed_array (&subiter, &value, &len);
//    dbus_message_iter_close_container(&args, &subiter);
    // Now Handle the cmd received
    if (mycb != NULL)
    {
    	method_reply=(*mycb)(value, len); // cb MUST copy the data, and don't block for long!!!!
    }
    else
    {
    	floge("dbustest.log","%s:%d callback is null, data not passed to client",__func__,__LINE__);
    }
//    printf("%s:%d method_reply->len=%d data=%s\n",__func__,__LINE__,method_reply->len,(char*)method_reply->data);

	if (!(reply = dbus_message_new_method_return(msg))){
    	goto fail;
    }
    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "y", &subiter);
//    dbus_message_iter_append_fixed_array(&subiter, DBUS_TYPE_BYTE, &(method_reply->data), method_reply->len);
    dbus_message_iter_append_fixed_array(&subiter, DBUS_TYPE_BYTE, &(method_reply->data), method_reply->len);
    dbus_message_iter_close_container(&args, &subiter);
//	dbus_message_append_args(reply,DBUS_TYPE_STRING, &(method_reply->data), DBUS_TYPE_INVALID);
fail:
	if (dbus_error_is_set(&err)) {
		if (reply)
			dbus_message_unref(reply);
		reply = dbus_message_new_error(msg, err.name, err.message);
		dbus_error_free(&err);
	}
	/*
	 * In any cases we should have allocated a reply otherwise it
	 * means that we failed to allocate one.
	 */
	if (!reply)
	{
		free(method_reply->data);
		free(method_reply);
		floge("dbustest.log","%s:%d Out Of Memory!",__func__,__LINE__);
		return ;
	}
	/* Send the reply which might be an error one too. */

	if (!dbus_connection_send(conn, reply, NULL))
	{
		free(method_reply->data);
		free(method_reply);
		floge("dbustest.log","%s:%d unable to send method reply!",__func__,__LINE__);
		return ;
	}
	dbus_message_unref(reply);
	free(method_reply->data);
	free(method_reply);
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}


static void handle_method_call (DBusMessage *msg, DBusConnection *conn, void *cb)
{
    DBusMessageIter rootIter;
    DBusMessageIter subIter;
    char *value = NULL;
    int len;
    callback mycb = (callback)cb;

    // read the arguments
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    if (!dbus_message_iter_init(msg, &rootIter))
      flogd("dbustest.log", "%s:%d Message has no arguments!",__func__,__LINE__);

    // TODO: Check these APIs to get the args from the msg.
    dbus_message_iter_recurse(&rootIter, &subIter);
    dbus_message_iter_get_fixed_array (&subIter, &value, &len);

    // Now Handle the cmd received
    if (mycb != NULL)
    {
        (*mycb)(value, len); // cb MUST copy the data, and don't block for long!!!!
    }
    else
    {
        floge("dbustest.log","%s:%d callback is null, data not passed to client",__func__,__LINE__);
    }
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}

static void dispatch(int fd, short ev, void *x)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    struct dbus_ctx *ctx = x;
    DBusConnection *c = ctx->conn;

    while (dbus_connection_get_dispatch_status(c) == DBUS_DISPATCH_DATA_REMAINS)
        dbus_connection_dispatch(c);
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}

static void handle_dispatch_status(DBusConnection *c,
                                   DBusDispatchStatus status, void *data)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    struct dbus_ctx *ctx = data;

    if (status == DBUS_DISPATCH_DATA_REMAINS) {
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 0,
        };
        event_add(&ctx->dispatch_ev, &tv);
    }
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}

static void handle_watch(int fd, short events, void *x)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    struct dbus_ctx *ctx = x;
    struct DBusWatch *watch = ctx->extra;

    unsigned int flags = 0;
    if (events & EV_READ)
        flags |= DBUS_WATCH_READABLE;
    if (events & EV_WRITE)
        flags |= DBUS_WATCH_WRITABLE;
    /*if (events & HUP)
        flags |= DBUS_WATCH_HANGUP;
    if (events & ERR)
        flags |= DBUS_WATCH_ERROR;*/

    if (dbus_watch_handle(watch, flags) == FALSE)
       flogd("dbustest.log","%s:%d dbus_watch_handle() failed\n",__func__,__LINE__);

    handle_dispatch_status(ctx->conn, DBUS_DISPATCH_DATA_REMAINS, ctx);
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}

static dbus_bool_t add_watch(DBusWatch *w, void *data)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
	if (!dbus_watch_get_enabled(w))
        return TRUE;

    struct dbus_ctx *ctx = data;
    ctx->extra = w;

    int fd = dbus_watch_get_unix_fd(w);
    unsigned int flags = dbus_watch_get_flags(w);
    short cond = EV_PERSIST;
    if (flags & DBUS_WATCH_READABLE)
        cond |= EV_READ;
    if (flags & DBUS_WATCH_WRITABLE)
        cond |= EV_WRITE;

    struct event *event = event_new(ctx->evbase, fd, cond, handle_watch, ctx);
    if (!event)
        return FALSE;

    event_add(event, NULL);

    dbus_watch_set_data(w, event, NULL);

    flogv("dbustest.log","%s:%d added dbus watch fd=%d watch=%p cond=%d\n",__func__,__LINE__, fd, w, cond);
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
    return TRUE;
}

static void remove_watch(DBusWatch *w, void *data)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    struct event *event = dbus_watch_get_data(w);

    if (event)
        event_free(event);

    dbus_watch_set_data(w, NULL, NULL);

    flogv("dbustest.log","%s:%d removed dbus watch watch=%p\n",__func__,__LINE__, w);
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}

static void toggle_watch(DBusWatch *w, void *data)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    flogv("dbustest.log","%s:%d toggling dbus watch watch=%p\n",__func__,__LINE__, w);

    if (dbus_watch_get_enabled(w))
        add_watch(w, data);
    else
        remove_watch(w, data);
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}

static void unregister_func(DBusConnection *connection, void *data)
{
}

static DBusHandlerResult message_func(DBusConnection *connection,
                                      DBusMessage *message, void *data)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    struct dbus_ctx *ctx = data;
    int i;

    /* handle DBus message */
    for (i = 0; i < ctx->args->num_pairs; i++)
    {

        //flogv("dbustest.log","is signal ? method %s", ctx->args->mthod_cb[i].method);
        if (dbus_message_is_signal(message, DBUS_INTERFACE_NAME, ctx->args->mthod_cb[i].method)) {
            flogv("dbustest.log","%s:%d handle method call %s",__func__,__LINE__, ctx->args->mthod_cb[i].method);
            handle_method_call(message, ctx->conn, ctx->args->mthod_cb[i].cb);
        }
        else if (dbus_message_is_method_call(message, DBUS_INTERFACE_NAME, ctx->args->mthod_cb[i].method)) {
            flogv("dbustest.log","%s:%d handle method call %s",__func__,__LINE__, ctx->args->mthod_cb[i].method);
            handle_method_call_with_reply(message, ctx->conn, ctx->args->mthod_cb[i].cb);
		}
    }

    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusObjectPathVTable dbus_vtable = {
    .unregister_function = unregister_func,
    .message_function = message_func,
};

static dbus_ctx_t *dbus_init(struct event_base *eb)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    DBusConnection *conn = rxconn;
    DBusError err;
    dbus_ctx_t *ctx = &gctx;
    char match_buffer[255];
    int len1;

    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);

    dbus_connection_set_exit_on_disconnect(conn, FALSE);

    ctx->conn = conn;
    ctx->evbase = eb;
    event_assign(&ctx->dispatch_ev, eb, -1, EV_TIMEOUT, dispatch, ctx);

    if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch,
                                             toggle_watch, ctx, NULL)) {
       flogd("dbustest.log","%s:%ddbus_connection_set_watch_functions() failed",__func__,__LINE__);
        goto out;
    }

    dbus_connection_set_dispatch_status_function(conn, handle_dispatch_status,
                                                 ctx, NULL);

    // add a rule for which messages we want to see
    len1 = strlen("type='signal',interface='");
    sprintf (match_buffer, "%s", "type='signal',interface='");
    sprintf (match_buffer+len1, "%s", DBUS_INTERFACE_NAME);
    sprintf(match_buffer+len1+strlen(DBUS_INTERFACE_NAME), "%s", "'");
    dbus_error_init(&err);
    dbus_bus_add_match(conn, match_buffer, &err); // see signals from the given interface
//    dbus_connection_flush(conn);
    if (dbus_error_is_set(&err)) {
       flogd("dbustest.log","%s:%d Match Error (%s)",__func__,__LINE__, err.message);
        dbus_error_free(&err);
        goto out;
    }
    dbus_error_free(&err);

    if (dbus_connection_register_object_path(conn, DBUS_OBJECT_NAME, &dbus_vtable,
                                             ctx) != TRUE) {
       flogd("dbustest.log","%s:%d failed to register object path\n",__func__,__LINE__);
        goto out;
    }

    return ctx;

out:
    if (conn) {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
    }
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
    return NULL;
}

void dbus_close(struct dbus_ctx *ctx)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    if (ctx && ctx->conn) {
        dbus_connection_flush(ctx->conn);
        dbus_connection_close(ctx->conn);
        dbus_connection_unref(ctx->conn);
        event_del(&ctx->dispatch_ev);
    }
    flogd("dbustest.log","%s:%d Exit",__func__,__LINE__);
}

/* Receive thread Main Loop */
static void *dbus_client_receive_thread (void *arg)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    rx_thread_args_t *args = (rx_thread_args_t *) arg;
    DBusError err;
    int rc;
    dbus_ctx_t *lctx;

    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    ev_base = event_base_new ();

    // initialise the error
    dbus_error_init(&err);
    lctx = dbus_init(ev_base);
    lctx->args = args;
    rc = event_base_dispatch(ev_base); 

    dbus_error_free(&err);
    flogd("dbustest.log","%s:%d %s: Exit (rc = %d)\n",__func__,__LINE__, rc);
}

/***************************** Interface Functions ***********************************/

/*
 * return:   0 on success, -1 on failure.
 */

/**
 * Initialize dbus.pass the unique application name as argument. this name will
 * be used to identify the dbus object and path name in the later calls.
 * @param appname unique application identifier name.
 * @return 0 on success error code otherwise.
 */
int dbus_client_init (char *appname)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    DBusError err;
    int rc = 0;
    printf("appname=%s\n",appname);
    pthread_mutex_lock (&init_dbus_mutex);
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    if (init_dbus_flag == false)
    {
        // initialize the errors
        dbus_error_init(&err);

        // connect to the system bus and check for errors
        txconn = dbus_bus_get(DBUS_BUS_SESSION, &err);
        if (dbus_error_is_set(&err))
        {
           flogd("dbustest.log","%s:%d txbus: Connection Error (%s)",__func__,__LINE__, err.message);
            dbus_error_free(&err);
            pthread_mutex_unlock (&init_dbus_mutex);
            return -1;
        }
        if (NULL == txconn)
        {
            dbus_error_free(&err);
            pthread_mutex_unlock (&init_dbus_mutex);
            return -1;
        }

        // request our name on the bus
        sprintf(txbusname + strlen(txbusname), "%s", appname);
        rc = dbus_bus_request_name(txconn, txbusname, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
        if (dbus_error_is_set(&err) || rc == -1)
        {
           flogd("dbustest.log", "%s:%d txbus: Name Error (%s)",__func__,__LINE__, err.message);
            dbus_error_free(&err);
            pthread_mutex_unlock (&init_dbus_mutex);
            return -1;
        }

        // connect to the system bus and check for errors
        rxconn = dbus_bus_get(DBUS_BUS_SESSION, &err);
        if (dbus_error_is_set(&err))
        {
           flogd("dbustest.log","%s:%d rxbus: Connection Error (%s)",__func__,__LINE__, err.message);
            dbus_error_free(&err);
            pthread_mutex_unlock (&init_dbus_mutex);
            return -1;
        }
        if (NULL == rxconn)
        {
            dbus_error_free(&err);
            pthread_mutex_unlock (&init_dbus_mutex);
            return -1;
        }

        // request our name on the bus
//        sprintf(rxbusname + strlen(rxbusname), "%d", getpid());
        sprintf(rxbusname + strlen(rxbusname), "%s", appname);
        printf("%s\n",rxbusname);
        rc = dbus_bus_request_name(rxconn, rxbusname, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
        if (dbus_error_is_set(&err) || rc == -1)
        {
           flogd("dbustest.log", "%s:%d rxbus: Name Error (%s)",__func__,__LINE__, err.message);
            dbus_error_free(&err);
            pthread_mutex_unlock (&init_dbus_mutex);
            return -1;
        }

        dbus_error_free(&err);
        init_dbus_flag = true;
    }
    else
    {
    	flogv("dbustest.log","%s:%d Dbus was already initialized",__func__,__LINE__);
    }

    flogd("dbustest.log","%s:%d Exit (no errors)",__func__,__LINE__);
    pthread_mutex_unlock (&init_dbus_mutex);
    return 0;
}

/* This API subscribes for a particular busname/interface/method. The callback would
 * be called once the subscribed information is available.
 *Note: subscribe = listen => server bus name (e.g., mqtt.server.cmdhndlr).
 *
 * method:    method name you wnat to listen to.
 * cb:        callback to be called once the subscribed information is available.
 * return:    0 on success, -1 on failure.
 */
int dbus_client_subscribe (int num_pairs, ...)
{
    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    int i;
    va_list vargs;
    rx_thread_args_t *args;

    if (num_pairs > 32)
        return -1;

    args = (rx_thread_args_t *) malloc (sizeof(rx_thread_args_t));
    if (args == NULL)
        return -1;

    args->num_pairs = num_pairs;
    va_start(vargs, num_pairs);
    for (i = 0; i < num_pairs; i++)
    {
        char *m = va_arg(vargs, char *);
        sprintf (args->mthod_cb[i].method, "%s", m);
        args->mthod_cb[i].cb = va_arg(vargs, void *);
    }
    va_end(vargs);

    if (pthread_create (&dbus_client_rx_thread, NULL, dbus_client_receive_thread, (void *)args) != 0)
    {
        floge("dbustest.log","%s:%d Client Error: Rx Thread creation failed; errno = %d (%s)",__func__,__LINE__, errno, strerror(errno));
        return -1;
    }

    /* TODO: Detach dbus_client_receive_thread. (or exit self in the created thread)
     * Cannot be joined here as it will block the client process.
     */
    flogd("dbustest.log","%s:%d exit",__func__,__LINE__);
    return 0;
}

/*
 * method:     method name
 * payload:    data that needs to be sent/published.
 * payloadlen: length of payload in bytes.
 * return:     0 on success, -1 on failure.
 */
int dbus_client_publish (char *method, void *payload, int payloadlen)
{
    DBusMessage *msg;
    DBusMessageIter args;
    DBusMessageIter subiter;
    DBusError err;

    flogd("dbustest.log","%s:%d Entry",__func__,__LINE__);
    char *pload = malloc(payloadlen);
    if (pload == NULL)
    {
       flogd("dbustest.log","%s:%d Malloc failed",__func__,__LINE__);
        return -1;
    }

    memcpy(pload, payload, payloadlen);

    // initialize the errors
    dbus_error_init(&err);

    // create a new method call and check for errors
    msg = dbus_message_new_signal(DBUS_OBJECT_NAME, // object name of the signal
                                  DBUS_INTERFACE_NAME, // interface name of the signal
                                  method); // name of the signal
    if (NULL == msg)
    {
       flogd("dbustest.log","%s:%d Message Null",__func__,__LINE__);
        free(pload);
        dbus_error_free(&err);
        return -1;
    }

    // append arguments
    dbus_message_iter_init_append(msg, &args);
    // For appending args, doc says to use the following 3 for container types.
    // We can view the cmd buffer as a char buffer and send
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "y", &subiter);
    dbus_message_iter_append_fixed_array(&subiter, DBUS_TYPE_BYTE, &pload, payloadlen);
    dbus_message_iter_close_container(&args, &subiter);

    // We probably don't need a reply
    flogv("dbustest.log","%s:%d dbus_connection_send called",__func__,__LINE__);
    if (!dbus_connection_send (txconn, msg, NULL)) {
       flogd("dbustest.log","%s:%d Out Of Memory!",__func__,__LINE__);
        dbus_message_unref(msg);
        free(pload);
        dbus_error_free(&err);
        return -1;
    }
    dbus_connection_flush(txconn);

    // free message
    dbus_message_unref(msg);

    free(pload);
    dbus_error_free(&err);
    flogd("dbustest.log","%s:%d Exit (no errors)",__func__,__LINE__);

    return 0;
}

/***************************************************************/
