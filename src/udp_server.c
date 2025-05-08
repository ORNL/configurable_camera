#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>

#include <glib.h>
#include <gio/gio.h>
#include <gst/gst.h>


#include "log.h"
#include "config.h"
#include "udp_server.h"
#include "pipeline.h"
#include "sensor.h"

#define DELIMITER " "


UDPListener *startUDPListener(ControlData *control_data) {
    /*
      Starts a socket at address and port specified in control_data
      listening for incoming UDP packets
    */

    UDPListener *udp_listener = (UDPListener *)malloc(sizeof(UDPListener));
    memset(udp_listener, 0, sizeof(UDPListener));

    // Make the udp inbound socket
    GError *socket_error = NULL;
    udp_listener->socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &socket_error);
    if (socket_error != NULL) {
        timestamp_prefix_err();
        g_printerr("Error creating UDP socket: %s\n",socket_error->message);
        g_error_free(socket_error);
        goto socket_err;
    }
    // Set to listen for broadcast messages
    g_socket_set_option(udp_listener->socket, SOL_SOCKET, SO_BROADCAST, TRUE, &socket_error);
    if (socket_error != NULL) {
        timestamp_prefix_err();
        g_printerr("Error setting broadcast listener setting: %s\n",socket_error->message);
        g_error_free(socket_error);
        goto address_err;
    }
    // Bind the udp inbound socket to the address and port from the config
    udp_listener->address = g_inet_socket_address_new_from_string(control_data->broadcastAddress,control_data->controlPort);
    GError *binding_error = NULL;
    gboolean bound = g_socket_bind(udp_listener->socket, udp_listener->address, TRUE, &binding_error);
    if (!bound){
        timestamp_prefix_err();
        g_printerr("Couldn't bind socket\n");
        return NULL;
    }
    if (binding_error != NULL){
        timestamp_prefix_err();
        g_printerr("Error binding to socket: %s\n", binding_error->message);
        g_error_free(binding_error);
        return NULL;
    }
    timestamp_prefix_log();
    g_print("Socket was bound successfully\n");

    // Get the file descriptor for the socket, use it to create a GIOChannel we can use to trigger
    // a callback when a UDP packet is received
    gint fd = g_socket_get_fd(udp_listener->socket);
    udp_listener->channel = g_io_channel_unix_new(fd);

    return udp_listener;

    address_err:
      g_object_unref(udp_listener->socket);
    socket_err:
      free(udp_listener);
      return NULL;
}

void stopUDPListener(UDPListener *udp_listener) {
    /*
      Safely close the socket on error or program shutdown
    */
    GError *err = NULL;
    GIOStatus status = g_io_channel_shutdown(udp_listener->channel, FALSE, &err);
    int tries = 0;

    // 3 tries to close it out
    while ((status != G_IO_STATUS_NORMAL) && (tries++ < 3)) {
        if (err != NULL) {
            timestamp_prefix_err();
            g_printerr("Error closing UDP listender channel: %s\n", err->message);
            g_error_free(err);
        }
        g_print("UDP listener channel unavailable to close.\n");
        status = g_io_channel_shutdown(udp_listener->channel, FALSE, &err);
    }
    timestamp_prefix_log();
    g_print("Closed UDP listener channel..\n");
    g_io_channel_unref(udp_listener->channel);
    g_object_unref(udp_listener->socket);
    free(udp_listener);
}

static void freeParsedTrigger(ParsedTrigger *pt) {
  if (pt != NULL) {
    free(pt->entire_string);
    free(pt->label);
    free(pt->command);
    free(pt->timestamp);
    free(pt);
  }
}

static ParsedTrigger *parseTrigger(ControlData *control_data, char* udp_string) {
    /*
        Parses a string into label, command, and timestamp tokens, validates that
        those tokens apply to this machine returns the parsed values in a 
        ParsedTrigger struct, or returns NULL if they do not apply or 
        are invalid.
    */

    ParsedTrigger *udp_received = (ParsedTrigger *)malloc(sizeof(ParsedTrigger));
    // set everything to zero so freeParsedTrigger frees NULLs on error
    memset(udp_received, 0, sizeof(ParsedTrigger));

    // copy the whole string into the struct
    udp_received->entire_string = strdup(udp_string);
    // make a copy of the trigger string that we can tokenize
    char *trigger_string = strdup(udp_received->entire_string);

    // sanity check that there are at least two delimiters in the string
    int delimiter_count = 0;
    for (int i = 0; trigger_string[i] != '\0'; i++){
        if (trigger_string[i] == DELIMITER[0])
            delimiter_count++;
    }
    if (delimiter_count < 2){
        goto invalid;
    }

    // tokenize the string into the fields we're looking for
    char *saveptr;
    udp_received->label = strdup(strtok_r(trigger_string, DELIMITER, &saveptr));
    udp_received->command = strdup(strtok_r(NULL, DELIMITER, &saveptr));
    udp_received->timestamp = strdup(strtok_r(NULL, DELIMITER, &saveptr));
    free(trigger_string);

	// check that the label matches one of the trigger keys
    char *valid[] = {"all", control_data->location, control_data->hostname};
    int num_strings = 3;
    for (int i = 0 ; i < num_strings ; i++) {
      if (!strcmp(udp_received->label, valid[i])) {
        timestamp_prefix_log();
        g_print("UDP message received: %s\n", udp_received->entire_string);
        return udp_received;
      }
    } 

    invalid:
      freeParsedTrigger(udp_received);
      return NULL;
}

static gboolean processHSTAppTrigger(PipelineData *pipeline_data, ParsedTrigger *parsed_trigger) {

    ControlData *control_data = pipeline_data->control_data;

    // format and create the recording directory  
    char *recording_dir;
    // config.h
    if (format_recording_directory(control_data, &recording_dir) == -1) {
    timestamp_prefix_err();
    g_printerr("Could not format recording directory.\n");
    free(recording_dir);
    return FALSE;
    }

    char *trigger_record_filepath =
        g_strdup_printf("%s%s_%s.txt", recording_dir, parsed_trigger->timestamp,
        parsed_trigger->command);

    timestamp_prefix_log();
    g_print("Writing trigger message to text file: %s\n", trigger_record_filepath);
    FILE *message_file;
    message_file = fopen(trigger_record_filepath, "w");
    fputs(parsed_trigger->entire_string, message_file);
    fclose(message_file);

    char *frame_level_metadata_filepath =
        g_strdup_printf("%s%s_metadata.csv", recording_dir, parsed_trigger->timestamp);
    
    new_csv(pipeline_data, frame_level_metadata_filepath);
    restart_file_limit_timer(pipeline_data);

    //TODO: error handling
    return TRUE;
}

gboolean udp_received_callback(GIOChannel *source, GIOCondition condition, gpointer data) {
    /*
    Callback for when a udp packet is received: read the string from the packet,
    parse it, process any commands and triggers
    TODO: Error handling -- need to decide on behavior
    */

    PipelineData *pipeline_data = data;
    ControlData *control_data = pipeline_data->control_data;

    // Read from the socket
    GError *error = NULL;
    gsize bytes_read;
    gchar buffer[1024];
    g_io_channel_read_chars(source, buffer, sizeof(buffer), &bytes_read, &error);
    buffer[bytes_read] = '\0';
    // Copy string from packet and tokenize
    ParsedTrigger *parsed_trigger = parseTrigger(control_data, buffer);

    if (parsed_trigger == NULL) {
        return G_SOURCE_CONTINUE;
    } else if (strcmp(parsed_trigger->command, "exposuretime") == 0) {
        float exposure_time = atof(parsed_trigger->timestamp);
        if (exposure_time > 0.1) {
            setSensorExposureTime(pipeline_data->source, exposure_time);
        }
    } else if (strcmp(parsed_trigger->command, "gain") == 0) {
        float gain = atof(parsed_trigger->timestamp);
        if (gain > 0.1) {
            setSensorGain(pipeline_data->source, gain);
        }
    } else if (strcmp(parsed_trigger->command, "start") == 0) {
        processHSTAppTrigger(pipeline_data, parsed_trigger);
		if (control_data->recordRaw) {
			record_raw(pipeline_data, parsed_trigger->timestamp);
		}
    } else if (strcmp(parsed_trigger->command, "stop") == 0) {
        processHSTAppTrigger(pipeline_data, parsed_trigger);
		if (control_data->recordRaw) {
			stop_record_raw(pipeline_data);
		}
    } else {
        timestamp_prefix_log();
        g_print("Invalid command or bad formatting: %s\n",
            parsed_trigger->command);
    }

    freeParsedTrigger(parsed_trigger);

    return G_SOURCE_CONTINUE;
}
