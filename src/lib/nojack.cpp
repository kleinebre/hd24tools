#include "nojack.h"
#ifndef NULL
#define NULL 0
#endif
jack_client_t *jack_client_new (const char *client_name) { return NULL; };
int jack_client_close (jack_client_t *client) { return 0; };
int jack_client_name_size(void) { return 0; };
int jack_internal_client_new (const char *client_name, const char *so_name,
			      const char *so_data) { return 0; };
void jack_internal_client_close (const char *client_name) { return; }
int jack_is_realtime (jack_client_t *client) { return 0; }
void jack_on_shutdown (jack_client_t *client, void (*function)(void *arg), void *arg) { return; }
int jack_set_process_callback (jack_client_t *client,
			       JackProcessCallback process_callback,
			       void *arg) {return 0; }
int jack_set_thread_init_callback (jack_client_t *client,
				   JackThreadInitCallback thread_init_callback,
				   void *arg) { return 0;}
int jack_set_freewheel_callback (jack_client_t *client,
				 JackFreewheelCallback freewheel_callback,
				 void *arg) { return 0;}
int jack_set_freewheel(jack_client_t* client, int onoff) { return 0;}

int jack_set_buffer_size (jack_client_t *client, jack_nframes_t nframes) {return 0;}
int jack_set_buffer_size_callback (jack_client_t *client,
				   JackBufferSizeCallback bufsize_callback,
				   void *arg) {return 0;};
int jack_set_sample_rate_callback (jack_client_t *client,
				   JackSampleRateCallback srate_callback,
				   void *arg) { return 0;};
int jack_transport_query(jack_client_t *x,__uint64* y) {return 0;}
int jack_set_port_registration_callback (jack_client_t *,
					 JackPortRegistrationCallback
					 registration_callback, void *arg) {return 0;};
int jack_set_graph_order_callback (jack_client_t *, JackGraphOrderCallback graph_callback, void *) {return 0;};

int jack_set_xrun_callback (jack_client_t *, JackXRunCallback xrun_callback, void *arg) { return 0;};
int jack_activate (jack_client_t *client) {return 0;};
int jack_deactivate (jack_client_t *client) {return 0;};
jack_port_t *jack_port_register (jack_client_t *client,
                                 const char *port_name,
                                 const char *port_type,
                                 unsigned long flags,
                                 unsigned long buffer_size) { return NULL; }
int jack_port_unregister (jack_client_t *, jack_port_t *) { return 0; }
void *jack_port_get_buffer (jack_port_t *, jack_nframes_t) { return NULL;}
const char *jack_port_name (const jack_port_t *port) { return NULL; }
const char *jack_port_short_name (const jack_port_t *port) { return NULL; }
int jack_port_flags (const jack_port_t *port) { return 0;}
const char *jack_port_type (const jack_port_t *port) { return 0;}

int jack_port_is_mine (const jack_client_t *, const jack_port_t *port) {return 0;};

int jack_port_connected (const jack_port_t *port);
int jack_port_connected_to (const jack_port_t *port,
			    const char *port_name);
const char **jack_port_get_connections (const jack_port_t *port);
const char **jack_port_get_all_connections (const jack_client_t *client,
					    const jack_port_t *port);
int  jack_port_tie (jack_port_t *src, jack_port_t *dst);
int  jack_port_untie (jack_port_t *port);
int jack_port_lock (jack_client_t *, jack_port_t *);
int jack_port_unlock (jack_client_t *, jack_port_t *);
jack_nframes_t jack_port_get_latency (jack_port_t *port);
jack_nframes_t jack_port_get_total_latency (jack_client_t *,
					    jack_port_t *port);
void jack_port_set_latency (jack_port_t *, jack_nframes_t);
int jack_port_set_name (jack_port_t *port, const char *port_name);
int jack_port_request_monitor (jack_port_t *port, int onoff);
int jack_port_ensure_monitor (jack_port_t *port, int onoff);
int jack_port_monitoring_input (jack_port_t *port);
int jack_connect (jack_client_t *,
		  const char *source_port,
		  const char *destination_port);
int jack_disconnect (jack_client_t *,
		     const char *source_port,
		     const char *destination_port);
int jack_port_disconnect (jack_client_t *, jack_port_t *);
int jack_port_name_size(void);

/**
 * @return the maximum number of characters in a JACK port type name
 * including the final NULL character.  This value is a constant.
 */
int jack_port_type_size(void);

/**
 * @return the sample rate of the jack system, as set by the user when
 * jackd was started.
 */
jack_nframes_t jack_get_sample_rate (jack_client_t *);

/**
 * @return the current maximum size that will ever be passed to the @a
 * process_callback.  It should only be used *before* the client has
 * been activated.  This size may change, clients that depend on it
 * must register a @a bufsize_callback so they will be notified if it
 * does.
 *
 * @see jack_set_buffer_size_callback()
 */
jack_nframes_t jack_get_buffer_size (jack_client_t *);

/**
 * @param port_name_pattern A regular expression used to select 
 * ports by name.  If NULL or of zero length, no selection based 
 * on name will be carried out.
 * @param type_name_pattern A regular expression used to select 
 * ports by type.  If NULL or of zero length, no selection based 
 * on type will be carried out.
 * @param flags A value used to select ports by their flags.  
 * If zero, no selection based on flags will be carried out.
 *
 * @return a NULL-terminated array of ports that match the specified
 * arguments.  The caller is responsible for calling free(3) any
 * non-NULL returned value.
 *
 * @see jack_port_name_size(), jack_port_type_size()
 */
const char **jack_get_ports (jack_client_t *, 
			     const char *port_name_pattern, 
			     const char *type_name_pattern, 
			     unsigned long flags);

/**
 * @return address of the jack_port_t named @a port_name.
 *
 * @see jack_port_name_size()
 */
jack_port_t *jack_port_by_name (jack_client_t *, const char *port_name);

/**
 * @return address of the jack_port_t of a @a port_id.
 */
jack_port_t *jack_port_by_id (const jack_client_t *client,
			      jack_port_id_t port_id);

/**
 * Old-style interface to become the timebase for the entire JACK
 * subsystem.
 *
 * @deprecated This function still exists for compatibility with the
 * earlier transport interface, but it does nothing.  Instead, see
 * transport.h and use jack_set_timebase_callback().
 *
 * @return ENOSYS, function not implemented.
 */
int  jack_engine_takeover_timebase (jack_client_t *);

/**
 * @return the time in frames that has passed since the JACK server
 * began the current process cycle.
 */
jack_nframes_t jack_frames_since_cycle_start (const jack_client_t *);

/**
 * @return an estimate of the current time in frames.  This is a
 * running counter, no significance should be attached to its value,
 * but it can be compared to a previously returned value.
 */
jack_nframes_t jack_frame_time (const jack_client_t *);

/**
 * @return the frame_time after the last processing of the graph
 * this is only to be used from the process callback. 
 *
 * This function can be used to put timestamps generated by 
 * jack_frame_time() in correlation to the current process cycle.
 */
jack_nframes_t jack_last_frame_time (const jack_client_t *client);


/**
 * @return the current CPU load estimated by JACK.  This is a running
 * average of the time it takes to execute a full process cycle for
 * all clients as a percentage of the real time available per cycle
 * determined by the buffer size and sample rate.
 */
float jack_cpu_load (jack_client_t *client);

/**
 * Set the directory in which the server is expected
 * to have put its communication FIFOs. A client
 * will need to call this before calling
 * jack_client_new() if the server was started
 * with arguments telling it to use a non-standard
 * directory.
 * 
 * @deprecated This function is deprecated.  Don't use in new programs
 * and remove it in old programs.
 */
void jack_set_server_dir (const char *path);

/**
 * @return the pthread ID of the thread running the JACK client side
 * code.
 */
pthread_t jack_client_thread_id (jack_client_t *);

/**
 * Display JACK error message.
 *
 * Set via jack_set_error_function(), otherwise a JACK-provided
 * default will print @a msg (plus a newline) to stderr.
 *
 * @param msg error message text (no newline at end).
 */
extern void (*jack_error_callback)(const char *msg);

/**
 * Set the @ref jack_error_callback for error message display.
 *
 * The JACK library provides two built-in callbacks for this purpose:
 * default_jack_error_callback() and silent_jack_error_callback().
 */
void jack_set_error_function (void (*func)(const char *));
__uint64 jack_get_current_transport_frame(void* client) { return 0; }
void jack_transport_stop (void * dummy) { return; };
void jack_transport_start (void * dummy) { return; };
void jack_transport_locate (void * dummy,int x) { return; };
//void* jack_port_register (void* dummy,const void* dum2,int q,int y,int z) {return NULL;};
#define JACK_DEFAULT_AUDIO_TYPE 0

