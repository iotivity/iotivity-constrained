//#include <stdio.h>
//#include "oc_api.h"
//#include "oc_ri.h"

#define RESOURCE_NAME "lightbulb"
#define RESOURCE_URI "/light/"
#define NUMRESOURCESTYPES 1
#define DEVICE 0
#define RESOURCE_LIGHT_TYPE "core.light"
#define MAX_LIGHT_RESOURCE_COUNT 4
#define NDEVICE 1
#define MAX_STRING 65 
#define FAN_INVISIBLE_URI "/device/fan-invisible"
extern char g_3DPrinter_RESOURCE_ENDPOINT[];
extern char g_AudioControls_RESOURCE_ENDPOINT[];
extern int g_AudioControls_nr_resource_types;
extern int quit;
pthread_mutex_t mutex;
pthread_cond_t cv;
struct timespec ts;
extern int g_3DPrinter_nr_resource_types;

extern void register_resources1(void);
int convert_if_string(char *interface_name);
void register_resources();
void signal_event_loop(void);
void signal_event_loop(void);
void handle_signal(int signal);
int app_init(void);
