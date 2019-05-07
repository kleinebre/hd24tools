/*
    Copyright (C) 2001 Paul Davis
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software 
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: jack.h,v 1.64 2004/07/15 03:07:28 trutkin Exp $
*/

#ifndef __jack_h__
#define __jack_h__

#include "config.h"
#define __int64 long long
#ifdef __cplusplus
extern "C" {
#endif
#define jack_client_t void
#define jack_nframes_t __uint64
#define jack_position_t __uint64
#define jack_default_audio_sample_t int
#define jack_port_t void*
#define JackPortIsOutput 0
#define JackPortIsInput 0
typedef int(*JackProcessCallback)(__uint64, int,void*);
#define JackThreadInitCallback void*
#define JackPortRegistrationCallback void*
#define JackGraphOrderCallback void*
#define JackXRunCallback void*
#define JackFreewheelCallback void*
#define JackBufferSizeCallback void*
#define JackSampleRateCallback void*
#define jack_port_id_t void*
#define jack_transport_state_t int
#define JackTransportStopped 0
#define pthread_t void*
jack_client_t *jack_client_new (const char *client_name);
int jack_client_close (jack_client_t *client);
int jack_client_name_size(void);
int jack_internal_client_new (const char *client_name, const char *so_name,
			      const char *so_data);
void jack_internal_client_close (const char *client_name);
int jack_is_realtime (jack_client_t *client);
void jack_on_shutdown (jack_client_t *client, void (*function)(void *arg), void *arg) ;
int jack_set_process_callback (jack_client_t *client,
			       JackProcessCallback process_callback,
			       void *arg) ;
int jack_set_thread_init_callback (jack_client_t *client,
				   JackThreadInitCallback thread_init_callback,
				   void *arg) ;
int jack_set_freewheel_callback (jack_client_t *client,
				 JackFreewheelCallback freewheel_callback,
				 void *arg) ;
int jack_set_freewheel(jack_client_t* client, int onoff) ;

int jack_set_buffer_size (jack_client_t *client, jack_nframes_t nframes) ;
int jack_set_buffer_size_callback (jack_client_t *client,
				   JackBufferSizeCallback bufsize_callback,
				   void *arg);
int jack_set_sample_rate_callback (jack_client_t *client,
				   JackSampleRateCallback srate_callback,
				   void *arg);
int jack_transport_query(jack_client_t *x,__uint64* y);
int jack_set_port_registration_callback (jack_client_t *,
					 JackPortRegistrationCallback
					 registration_callback, void *arg) ;
int jack_set_graph_order_callback (jack_client_t *, JackGraphOrderCallback graph_callback, void *) ;

int jack_set_xrun_callback (jack_client_t *, JackXRunCallback xrun_callback, void *arg) ;
int jack_activate (jack_client_t *client) ;
int jack_deactivate (jack_client_t *client) ;
jack_port_t *jack_port_register (jack_client_t *client,
                                 const char *port_name,
                                 const char *port_type,
                                 unsigned long flags,
                                 unsigned long buffer_size) ;
int jack_port_unregister (jack_client_t *, jack_port_t *) ;
void *jack_port_get_buffer (jack_port_t *, jack_nframes_t) ;
const char *jack_port_name (const jack_port_t *port) ;
const char *jack_port_short_name (const jack_port_t *port) ;
int jack_port_flags (const jack_port_t *port) ;
const char *jack_port_type (const jack_port_t *port) ;

int jack_port_is_mine (const jack_client_t *, const jack_port_t *port) ;

/** 
 * @return number of connections to or from @a port.
 *
 * @pre The calling client must own @a port.
 */
int jack_port_connected (const jack_port_t *port);

/**
 * @return TRUE if the locally-owned @a port is @b directly connected
 * to the @a port_name.
 *
 * @see jack_port_name_size()
 */
int jack_port_connected_to (const jack_port_t *port,
			    const char *port_name);

/**
 * @return a null-terminated array of full port names to which the @a
 * port is connected.  If none, returns NULL.
 *
 * The caller is responsible for calling free(3) on any non-NULL
 * returned value.
 *
 * @param port locally owned jack_port_t pointer.
 *
 * @see jack_port_name_size(), jack_port_get_all_connections()
 */   
const char **jack_port_get_connections (const jack_port_t *port);

/**
 * @return a null-terminated array of full port names to which the @a
 * port is connected.  If none, returns NULL.
 *
 * The caller is responsible for calling free(3) on any non-NULL
 * returned value.
 *
 * This differs from jack_port_get_connections() in two important
 * respects:
 *
 *     1) You may not call this function from code that is
 *          executed in response to a JACK event. For example,
 *          you cannot use it in a GraphReordered handler.
 *
 *     2) You need not be the owner of the port to get information
 *          about its connections. 
 *
 * @see jack_port_name_size()
 */   
const char **jack_port_get_all_connections (const jack_client_t *client,
					    const jack_port_t *port);

/**
 * A client may call this on a pair of its own ports to 
 * semi-permanently wire them together. This means that
 * a client that wants to direct-wire an input port to
 * an output port can call this and then no longer
 * have to worry about moving data between them. Any data
 * arriving at the input port will appear automatically
 * at the output port.
 *
 * The 'destination' port must be an output port. The 'source'
 * port must be an input port. Both ports must belong to
 * the same client. You cannot use this to tie ports between
 * clients. That is what a connection is for.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int  jack_port_tie (jack_port_t *src, jack_port_t *dst);

/**
 * This undoes the effect of jack_port_tie(). The port
 * should be same as the 'destination' port passed to
 * jack_port_tie().
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int  jack_port_untie (jack_port_t *port);

/**
 * A client may call this function to prevent other objects
 * from changing the connection status of a port. The port
 * must be owned by the calling client.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_port_lock (jack_client_t *, jack_port_t *);

/**
 * This allows other objects to change the connection status of a port.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_port_unlock (jack_client_t *, jack_port_t *);

/** 
 * @return the time (in frames) between data being available or
 * delivered at/to a port, and the time at which it arrived at or is
 * delivered to the "other side" of the port.  E.g. for a physical
 * audio output port, this is the time between writing to the port and
 * when the signal will leave the connector.  For a physical audio
 * input port, this is the time between the sound arriving at the
 * connector and the corresponding frames being readable from the
 * port.
 */
jack_nframes_t jack_port_get_latency (jack_port_t *port);

/**
 * The maximum of the sum of the latencies in every
 * connection path that can be drawn between the port and other
 * ports with the @ref JackPortIsTerminal flag set.
 */
jack_nframes_t jack_port_get_total_latency (jack_client_t *,
					    jack_port_t *port);

/**
 * The port latency is zero by default. Clients that control
 * physical hardware with non-zero latency should call this
 * to set the latency to its correct value. Note that the value
 * should include any systemic latency present "outside" the
 * physical hardware controlled by the client. For example,
 * for a client controlling a digital audio interface connected
 * to an external digital converter, the latency setting should
 * include both buffering by the audio interface *and* the converter. 
 */
void jack_port_set_latency (jack_port_t *, jack_nframes_t);

/**
 * Modify a port's short name.  May be called at any time.  If the
 * resulting full name (including the @a "client_name:" prefix) is
 * longer than jack_port_name_size(), it will be truncated.
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int jack_port_set_name (jack_port_t *port, const char *port_name);

/**
 * If @ref JackPortCanMonitor is set for this @a port, turn input
 * monitoring on or off.  Otherwise, do nothing.
 */
int jack_port_request_monitor (jack_port_t *port, int onoff);

/**
 * If @ref JackPortCanMonitor is set for this @a port_name, turn input
 * monitoring on or off.  Otherwise, do nothing.
 *
 * @return 0 on success, otherwise a non-zero error code.
 *
 * @see jack_port_name_size()
 */
int jack_port_request_monitor_by_name (jack_client_t *client,
				       const char *port_name, int onoff);

/**
 * If @ref JackPortCanMonitor is set for a port, this function turns
 * on input monitoring if it was off, and turns it off if only one
 * request has been made to turn it on.  Otherwise it does nothing.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_port_ensure_monitor (jack_port_t *port, int onoff);

/**
 * @return TRUE if input monitoring has been requested for @a port.
 */
int jack_port_monitoring_input (jack_port_t *port);

/**
 * Establish a connection between two ports.
 *
 * When a connection exists, data written to the source port will
 * be available to be read at the destination port.
 *
 * @pre The port types must be identical.
 *
 * @pre The @ref JackPortFlags of the @a source_port must include @ref
 * JackPortIsOutput.
 *
 * @pre The @ref JackPortFlags of the @a destination_port must include
 * @ref JackPortIsInput.
 *
 * @return 0 on success, EEXIST if the connection is already made,
 * otherwise a non-zero error code
 */
int jack_connect (jack_client_t *,
		  const char *source_port,
		  const char *destination_port);

/**
 * Remove a connection between two ports.
 *
 * @pre The port types must be identical.
 *
 * @pre The @ref JackPortFlags of the @a source_port must include @ref
 * JackPortIsOutput.
 *
 * @pre The @ref JackPortFlags of the @a destination_port must include
 * @ref JackPortIsInput.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_disconnect (jack_client_t *,
		     const char *source_port,
		     const char *destination_port);

/**
 * Perform the same function as jack_disconnect() using port handles
 * rather than names.  This avoids the name lookup inherent in the
 * name-based version.
 *
 * Clients connecting their own ports are likely to use this function,
 * while generic connection clients (e.g. patchbays) would use
 * jack_disconnect().
 */
int jack_port_disconnect (jack_client_t *, jack_port_t *);

/**
 * @return the maximum number of characters in a full JACK port name
 * including the final NULL character.  This value is a constant.
 *
 * A port's full name contains the owning client name concatenated
 * with a colon (:) followed by its short name and a NULL
 * character.
 */
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
__uint64 jack_get_current_transport_frame(void* client) ;
void jack_transport_stop (void * dummy) ;
void jack_transport_start (void * dummy) ;
void jack_transport_locate (void * dummy,int x) ;
#define JACK_DEFAULT_AUDIO_TYPE 0
#ifdef __cplusplus
}
#endif

#endif /* __jack_h__ */
