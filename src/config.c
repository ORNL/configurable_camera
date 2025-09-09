#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "addresschecker.h"

#define FILE_BUFFER_SIZE 4096
#define PIPELINE_TXT_LINES 4
#define PIPELINE_LINE_SIZE 256
#define PIPELINE_BUFFER_SIZE PIPELINE_TXT_LINES * PIPELINE_LINE_SIZE

HardwareScripts *read_HardwareScript(cJSON *config) {
  /*
  HardwareScripts is a linked list data structure
  This function allocates and reads from JSON one element of the list
  For the full list logic, see read_HardwareScripts
  */
  HardwareScripts *hardware_script = malloc(sizeof(HardwareScripts));
  memset(hardware_script, 0, sizeof(HardwareScripts));
  for (cJSON *ob = config->child; ob != NULL; ob = ob->next) {
    if (strcmp(ob->string, "Name") == 0) {
      hardware_script->name = strdup(ob->valuestring);
    } else if (strcmp(ob->string, "Filepath") == 0) {
      hardware_script->filePath = strdup(ob->valuestring);
    } else if (strcmp(ob->string, "Listener Port") == 0) {
      hardware_script->listenerPort = strdup(ob->valuestring);
    } else if (strcmp(ob->string, "Device IP") == 0) {
      hardware_script->deviceIP = strdup(ob->valuestring);
    } else if (strcmp(ob->string, "Device Port") == 0) {
      hardware_script->devicePort = strdup(ob->valuestring);
    } else if (strcmp(ob->string, "Capabilities") == 0) {
      // Copy the whole capabilities JSON to the hardware scripts
      // so they can be broadcast
      hardware_script->capabilities = ob;
    } else {
      printf("Unknown field in config.json, Hardware Scripts: %s\n", ob->string);
      free_HardwareScripts(hardware_script);
      return NULL;
    }
  }

  return hardware_script;
}

HardwareScripts *read_HardwareScripts(cJSON *array) {
  /*Iteritively parses raw json object data to create a list of structs*/

  // If there are no hardware scripts
  if (array->child == NULL)
    return NULL;

  // encode the first, then iterate over the linked list
  HardwareScripts *header = read_HardwareScript(array->child);
  HardwareScripts *itr = header;

  for (cJSON *ob = array->child->next; ob != NULL; ob = ob->next) {
    itr->next = read_HardwareScript(ob);
    itr = itr->next;
  }
  itr->next = NULL;

  return header;
}

ControlData *read_ControlData(cJSON *config) {
  ControlData *control_data = malloc(sizeof(ControlData));
  memset(control_data, 0, sizeof(ControlData));

  for (cJSON *ob = config->child; ob != NULL; ob = ob->next) {
    if (strcmp(ob->string, "Record Raw") == 0) {
      control_data->recordRaw = ob->valueint;
    } else if (strcmp(ob->string, "Raw Storage") == 0) {
      int len = strlen(ob->valuestring);
      // enforce that the recording directory filepath ends with a /
      if ((ob->valuestring)[len-1] != '/') {
        // +1 for the '\0', +1 for the '/' -- malloc(len + 2)
        control_data->rawStorage = (char *) malloc(len + 2);
        strcpy(control_data->rawStorage, ob->valuestring);
        strcpy(control_data->rawStorage + len, "/");
      } else {
        control_data->rawStorage = strdup(ob->valuestring);
      }
    } else if (strcmp(ob->string, "Raw Time Limit") == 0) {
      control_data->rawTimeLimit = ob->valueint;
    } else if (strcmp(ob->string, "Control IP") == 0) {
      control_data->controlIP = strdup(ob->valuestring);
      if (replace_with_address(&(control_data->controlIP), &(control_data->broadcastAddress)) != 0) {
        goto err;
      }
    } else if (strcmp(ob->string, "Control Port") == 0) {
      control_data->controlPort = ob->valueint;
    } else if (strcmp(ob->string, "Status Broadcast Port") == 0) {
      control_data->statusBroadcastPort = ob->valueint;
    } else if (strcmp(ob->string, "RTSP Address") == 0) {
      control_data->RTSPaddress = strdup(ob->valuestring);
      if (replace_with_address(&(control_data->RTSPaddress), NULL) !=0) {
        goto err;
      }
    } else if (strcmp(ob->string, "Hardware Scripts") == 0) {
      control_data->head = read_HardwareScripts(ob);
    } else {
      printf("Unknown field in config.json, Control Data: %s\n", ob->string);
      goto err;
    }
  }

  return control_data;

  err:
    printf("Failed to read Control Data.\n");
    free_ControlData(control_data);
    return NULL;
}

Metadata *read_Metadata(cJSON *config) {
  /*Parses raw json object data to store in strut. Mallocing every object and
  * struct, so needs to be freed*/

  Metadata *metadata = malloc(sizeof(Metadata));
  memset(metadata, 0, sizeof(Metadata));
  for (cJSON *ob = config->child; ob != NULL; ob = ob->next) {
    if (strcmp(ob->string, "Name") == 0) {
      metadata->name = strdup(ob->valuestring);
    } else if (strcmp(ob->string, "Location") == 0) {
      metadata->location = strdup(ob->valuestring);
    } else if (strcmp(ob->string, "Range") == 0) {
      metadata->range = ob->valueint;
    } else if (strcmp(ob->string, "Yaw") == 0) {
      metadata->yaw = ob->valueint;
    } else if (strcmp(ob->string, "Elevated") == 0) {
      metadata->elevated = ob->valueint;
    } else if (strcmp(ob->string, "Lens Name") == 0) {
      metadata->lensName = strdup(ob->valuestring);
    } else if (strcmp(ob->string, "Focal Length") == 0) {
      metadata->focalLength = ob->valueint;
    } else if (strcmp(ob->string, "Collection ID") == 0) {
      metadata->collectionID = strdup(ob->valuestring);
    } else {
      printf("Unknown field in config.json, Metadata: %s\n", ob->string);
      free_Metadata(metadata);
      return NULL;
    }
  }

  return metadata;
}

void free_Metadata(Metadata *data) {
  free(data->name);
  free(data->location);
  free(data->lensName);
  free(data->collectionID);
  free(data);
  data = NULL;
}

void free_ControlData(ControlData *data) {
  free_HardwareScripts(data->head);
  cJSON_Delete(data->json);
  data->json = NULL;
  // Don't need to free location because it points to Metadata.location
  free(data->controlIP);
  free(data->RTSPaddress);
  free(data->rawStorage);
  free(data->hostname);
  free(data->broadcastAddress);
  free(data);
}

void free_HardwareScripts(HardwareScripts *data) {

  while (data != NULL) {
    HardwareScripts *next = data->next;
    free(data->name);
    free(data->filePath);
    free(data->listenerPort);
    free(data->deviceIP);
    free(data->devicePort);
    free(data);
    data = next; 
  }
}

int readConfigFile(char *config_filepath, ControlData **control_data_ptr, Metadata **metadata_ptr) {
  /*Reads the JSON config file at config_filepath, allocates and writes control_data_ptr and metadata_ptr,
    and returns a cJSON pointer that must be freed with cJSON_Delete after ControlData is freed*/

  // open the file
  FILE *fp = fopen(config_filepath, "r");
  if (fp == NULL) {
    printf("Error: Unable to open the file.\n");
    return -1;
  }

  // read the file contents into a string
  char file_buffer[FILE_BUFFER_SIZE];
  memset(file_buffer, '\0', FILE_BUFFER_SIZE);
  int read = fread(file_buffer, sizeof(char), FILE_BUFFER_SIZE, fp);
  fclose(fp);

  if (read <= 0) {
    printf("Failed to read from config file.\n");
    return -1;
  }

  // parse the JSON data
  cJSON *json = cJSON_Parse(file_buffer);
  if (json == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      printf("Error: %s\n", error_ptr);
    }
    return -1;
  }

  // access the JSON data
  cJSON *json_control_data = cJSON_GetObjectItemCaseSensitive(json, "Control Data");

  if (json_control_data == NULL) {
    printf("Failed to get Control Data object from config.json.\n");
    goto failed_control;
  }

  *control_data_ptr = read_ControlData(json_control_data);

  if (*control_data_ptr == NULL) {
    printf("Failed to parse ControlData.\n");
    goto failed_control;
  }

  cJSON *metadata_json = cJSON_GetObjectItemCaseSensitive(json, "Metadata");

  if (metadata_json == NULL) {
    printf("mData is NULL\n");
    goto failed_metadata;
  }

  *metadata_ptr = read_Metadata(metadata_json);

  if (*metadata_ptr == NULL) {
    printf("Metadata couldn't be encoded\n");
    goto failed_metadata;
  }
  
  // Set up additional control data
  // duplicate location value because this will need to be
  // recorded as metadata and used to filter trigger messages
  (*control_data_ptr)->location = (*metadata_ptr)->location;
  // Get Jetson hostname
  char buffer[256];
  memset(buffer, '\0', 256);
  int hostname_err = gethostname(buffer, 255);
  if (hostname_err != 0) {
    printf("Error getting hostname.\n");
    goto failed;
  }
  (*control_data_ptr)->hostname = strdup(buffer);

  if (!isValidIp4((*control_data_ptr)->controlIP)) {
    printf("Invalid network IP address: %s\n", (*control_data_ptr)->controlIP);
    goto failed;
  }
  if (!isValidIp4((*control_data_ptr)->broadcastAddress)) {
    printf("Invalid broadcast IP address: %s\n", (*control_data_ptr)->broadcastAddress);
    goto failed;
  }
  if (!isValidIp4((*control_data_ptr)->RTSPaddress)) {
    printf("Invalid rtsp IP address: %s\n", (*control_data_ptr)->RTSPaddress);
    goto failed;
  }

  return 0;

  failed:
    free_Metadata(*metadata_ptr);
  failed_metadata:
    free_ControlData(*control_data_ptr);
  failed_control:
    cJSON_Delete(json);
    return -1;
}

void printConfigSummary(ControlData *control_data, Metadata *metadata, HardwareScripts *hardware_scripts) {
  // TODO -- finish this
  printf(" ----- Configurable Camera Settings ----- \n");
  printf("Name: %s\n", metadata->name);
  printf("Location: %s\n", control_data->location);
  printf("Range: %d\n", metadata->range);
  printf("Jetson: %s\n", control_data->hostname);
  printf("Lens: %s\n", metadata->lensName);
  printf(" -------- \n");
  printf("Network IP: %s\n", control_data->controlIP);
  printf("Control Port: %i\n", control_data->controlPort);
  printf("Status Broadcast Port: %i\n", control_data->statusBroadcastPort);
  printf("RTSP Address: %s\n", control_data->RTSPaddress);
  printf("Raw Recording: %i\n", control_data->recordRaw); 
  printf("Raw Storage Path: %s\n", control_data->rawStorage);
  printf("File Time Limit: %i seconds\n", control_data->rawTimeLimit);
  printf("Hardware:\n");
  HardwareScripts *next = control_data->head;
  while (next != NULL) {
    printf(" -- %s\n", next->name);
    next = next->next;
  }
  printf(" -------- \n\n");
}

int format_recording_directory(ControlData *control_data, char **filepath_ptr) {
  /*
  Format the current recording directory from info in control_data.
  The filepath_ptr will point to a string filepath ending with '/'
  Create the directory named by *filepath_ptr, return 0
  If directory creation failed, return -1
  */
  char *recording_dir = control_data->rawStorage;

  int recording_dir_len = strlen(recording_dir); // recording_dir is enforced to end with '/'
  int hostname_len = strlen(control_data->hostname);

  // allocate length for "<recording_dir/><date>/<hostname>/\0"
  int filepath_len = recording_dir_len + DATE_LEN + 1 + hostname_len + 2;

  *filepath_ptr = (char *) malloc(sizeof(char) * filepath_len);

  char *filepath = *filepath_ptr;
  // set all to null so that intermediate directories can be created from valid substrings
  memset(filepath, '\0', filepath_len);

  char *head = strcpy(filepath, recording_dir);
  head += recording_dir_len;

  // format the date directory
  struct tm time_struct;
  time_t ts = time(NULL);

  gmtime_r(&ts, &time_struct);

  char formatted_time[DATE_LEN + 1];
  int written = strftime(formatted_time, DATE_LEN + 1, "%Y%m%d", &time_struct);
  if (written != DATE_LEN) {
    free(*filepath_ptr);
    return -1;
  }

  strcpy(head, formatted_time);
  head += DATE_LEN;
  strcpy(head++, "/");

  // create the date directory if it doesn't exist
  struct stat st = {0};
  if (stat(filepath, &st) == -1) {
      mkdir(filepath, 0755);
  }

  strcpy(head, control_data->hostname);
  head += strlen(control_data->hostname);
  strcpy(head++, "/");

  // create the hostname directory if it doesn't exist
  if (stat(filepath, &st) == -1) {
      mkdir(filepath, 0755);
  }

  // if the directory doesn't exist, return -1
  if (stat(filepath, &st) == -1) {
    free(*filepath_ptr);
    return -1;
  }

  return 0;

}

int replace_with_address(char** interface_name, char** broadcast_address) {
  /*
  Iterates through network interfaces, checks for AF_INET type addresses
  and assigns their IPv4 address to 
  the string pointed to by interface_name
  if the string matches one of the interfaces.
  Also fills out the corresponding broadcast address if not a null pointer
  */
  struct ifaddrs *ifaddr;
  int family, s;
  char host[IP_STR_MAX_LEN + 1];
  char broadcast[IP_STR_MAX_LEN + 1];

  if (getifaddrs(&ifaddr) == -1) {
    printf("Failed to get network interface addresses\n");
    return -1;
  }

  for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    if (family == AF_INET) {
      s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, IP_STR_MAX_LEN, NULL, 0, NI_NUMERICHOST); 
      if (s != 0) {
        printf("getnameinfo() failed to get network address.\n");
        freeifaddrs(ifaddr);
        return -1;
      }
      if (broadcast_address != NULL) {
	s = getnameinfo(ifa->ifa_broadaddr, sizeof(struct sockaddr_in), broadcast, IP_STR_MAX_LEN, NULL, 0, NI_NUMERICHOST);
	if (s != 0) {
	  printf("getnameinfo() failed to get broadcast address.\n");
	  freeifaddrs(ifaddr);
	  return -1;
	}
      }
      if (strcmp(ifa->ifa_name, *interface_name) == 0) {
        free(*interface_name);
        *interface_name = strdup(host);
      }
      if (strcmp(*interface_name, host) == 0) {
	if (broadcast_address != NULL) {
	  *broadcast_address = strdup(broadcast);
	}
      }
    }
  }
  freeifaddrs(ifaddr);
  return 0;
}

int copy_file(const char *from, const char *to)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}

char *gst_pipeline_txt_gen(const char *filepath, int record_raw) {
  /*
  Reads the pipeline description from the specified filepath
  Concatenates specific lines based on rtsp/raw
  Allocates memory for a pipeline description string
  */
  FILE *file = fopen(filepath, "r");

  if (file == NULL) {
    printf("Failed to open file at %s\n", filepath);
    return NULL;
  }
  char line[256];
  char buffer[1024];
  char *head = buffer;

  for (int i = 0; i < PIPELINE_TXT_LINES; i++) {
    if (fgets(line, sizeof(line), file) == NULL) {
      goto file_err;
    }

    if (feof(file)) {
      printf("End of file reached unexpectedly.\n");
      goto file_err;
    }

    if (ferror(file)) {
      goto file_err;
    }

    // skip the second line (tee, queue, sink and rtsp tee link) if rtsp only
    if (i != 1 || record_raw) {
      // strip the newline
      line[strcspn(line, "\n")] = 0;
      strcpy(head, line);
      head += strlen(line);
    }
  }
  fclose(file);

  // end the string and copy it to memory, then return
  head = '\0';
  char *pipeline_string = strdup(buffer);
  return pipeline_string;

  file_err:
    printf("Error reading from pipeline description file: %s.\n", filepath);
    fclose(file);
    return NULL;
}
