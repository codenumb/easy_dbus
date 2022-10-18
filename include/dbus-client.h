#ifndef _DBUS_CLIENT_H
#define _DBUS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct method_reply
{
	void *data;
	int len;
}method_reply_t;
/* 
 * return:   0 on success, -1 on failure.
 */
extern int dbus_client_init (char *appname);

/* This API subscribes for a particular busname/interface/method. The callback would
 * be called once the subscribed information is available.
 *Note: subscribe = listen => server bus name (e.g., mqtt.server.cmdhndlr).
 *
 * method:    method name you wnat to listen to.
 * cb:        callback to be called once the subscribed information is available.
 * return:    0 on success, -1 on failure.
 */

extern int dbus_client_subscribe (int num_pairs, ...);

/* 
 * method:     method name
 * payload:    data that needs to be sent/published.
 * payloadlen: length of payload in bytes.
 * return:     0 on success, -1 on failure.
 */
extern int dbus_client_publish (char *method, 
                                void *payload,
                                int payloadlen);
extern method_reply_t *dbus_client_init_reply_buff(int len);
extern int  dbus_client_call_remote_method_with_reply(char *busname,
													  char *method,
													  void *payload,
													  int payloadlen,
													  void **reply,
													  int *replylen );
#ifdef __cplusplus
}
#endif

#endif // _DBUS_CLIENT_H

