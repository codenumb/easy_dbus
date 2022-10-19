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

/**
 * Function to initialize the dbus_client library.which establish a connection with the dbus bus.
 * @param appname the unique application name.other processes use this name to communicate.
 * @return 0 on success otherwise error code.
 */
extern int dbus_client_init (char *appname);

/**
 * Use this function to subscribe signals or methods. pass method/signals and
 * call back functions as comma separated pairs.list of pairs can be given with comma 
 * separation. 
 * eg  dbus_client_subscribe(3,"sig1",sig1_callback,"sig2",sig2_callback,"method1",method1_callback).
 * @param num_pairs number of pairs.
 * @return 0 on success, -1 on failure.
 */
extern int dbus_client_subscribe (int num_pairs, ...);

/**
 * Function to broadcast a signal.
 * @param method name of the signal.
 * @param payload data to be passed to listeners.
 * @param payloadlen length of the data in bytes.
 * @return 0 on success, -1 on failure.
 */
extern int dbus_client_publish (char *method, 
                                void *payload,
                                int payloadlen);

/**
 * Use this function to initialize the dbus method reply buffer.
 * no need to call de-init.
 * @param len required buffer legth in bytes.
 * @return -ve error code on failure and method_reply on success.
 */
extern method_reply_t *dbus_client_init_reply_buff(int len);

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

