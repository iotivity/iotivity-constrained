/******************************************************************
 *
 * Copyright 2018 GRANITE RIVER LABS All Rights Reserved.
 *
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "oc_api.h"
#include <string.h>
#include "Certification.h"
#include "oc_collection.h"
#include "oc_rep.h"

static bool g_addInvisibleResource = false;
static bool g_isTempResourceCreated = false;
static bool g_isManyLightCreated = false;
static bool g_isInvisibleResourceCreated = false;
static bool g_createResourceWithURL = false;
static bool g_isAirConDeviceCreated = false;
static bool g_binaryswitch_valuecb = false;
static bool resource_found = false;

static char g_binaryswitch_RESOURCE_PROPERTY_NAME_value[] = "value";
static char* UserResourceType_input = NULL;
static char a_light[MAX_URI_LENGTH];


static int s_generalQuit =0;

static oc_endpoint_t *resource_server = NULL;
static oc_endpoint_t *light_server;
static oc_collection_t* collectionPointer;

static void createResource();
static void createInvisibleResource();
static void createResourceWithUrl();
static void createManyLightResources();
static oc_collection_t* createGroupResource();
static void deleteAllResources();
static void deleteCreatedGroup();
static void sendPOSTRequest_partialUpdate_userInput();
static void findGroup(char *);
static oc_collection_t* updateGroup();
static void updateLocalResourceManually();
static void createSingleAirConResource();
static void handleMenu();
static void selectMenu(int);
static int app_init1();
static void findAllResources();
static void discoverIntrospection();
static void sendGetRequest();
static void findResource_UserResType(char *);
static bool is_resource_found();
static void observe_request();
static void stop_observe();

pthread_mutex_t mutex;
pthread_mutex_t app_mutex;
/*It display the available options */
void showMenu(int argc, char* argv[])
{

    printf("\n\t-----------------------------------------------------\n");
    printf("\tPlease Select an option from the menu and press Enter\n" );
    printf("\t-----------------------------------------------------\n");
    printf("\t\t0   : Quit Certification App\n" );
    printf("\n\tServer Operations:\n" );
    printf("\t\t1   : Create Normal Resource\n" );
    printf("\t\t2   : Create Invisible Resource\n" );
    printf("\t\t3   : Create Resource With Complete URL\n" );
    printf("\t\t4   : Create Secured Resource\n" );
    printf("\t\t5   : Create %d Light Resources\n",MAX_LIGHT_RESOURCE_COUNT );
    printf("\t\t6   : Create Group Resource\n" );
    printf("\t\t7   : Delete All Resources\n" );
    printf("\t\t8   : Delete Created Group\n" );
    printf("\n\tClient Operations:\n" );
    printf("\t\t9   : Find Introspection\n" );
    printf("\t\t11  : Find specific type of resource\n" );
    printf("\t\t12  : Find All Resources\n" );
    printf("\t\t17  : Send GET Request\n" );
    printf("\t\t22  : Send POST Request - Partial Update - User Input\n" );
    printf("\t\t25  : Observe Resource - Retrieve Request with Observe\n" );
    printf("\t\t26  : Cancel Observing Resource\n" );
    printf("\t\t31  : Find Group\n" );
    printf("\t\t33  : Update Group\n" );
    printf("\t\t34  : Update Local Resource Manually\n" );
    printf("\t\t107 : Create Air Conditioner Single Resource\n" );

    int choice;

    if (argc > 4) {
        for (int i = 5; i < argc; i++) {
                choice = atoi(argv[i]);
                selectMenu(choice);
        }
    }
}

/*Perform the selected operation*/
void selectMenu(int choice)
{
    switch(choice) {
        case 1:
            createResource();
            break;

        case 2:
            createInvisibleResource();
            break;

        case 3:
            createResourceWithUrl();
            break;

        case 4:
            printf("By default Resource created in Secure mode");
            break;

        case 5:
            createManyLightResources();
            break;

        case 6:
            if(g_isTempResourceCreated == true)
                collectionPointer = createGroupResource();
            else
                printf("\n!!!!!Please create resource first!!!!!\n");
            break;

        case 7:
            deleteAllResources();
            break;

        case 8:
            deleteCreatedGroup();
            break;

        case 9:
            discoverIntrospection();
            break;

        case 11:
            printf("Please type the Resource Type to find, then press Enter: ");
            unsigned int count_restype = 25;
            UserResourceType_input = malloc((size_t)count_restype);
            int numResTypes = scanf("%s", UserResourceType_input);
            if (numResTypes) {
                printf("\nuserResourceType entered is %s\n",UserResourceType_input );
                findResource_UserResType(UserResourceType_input);
            }
            break;

        case 12:
            findAllResources();
            break;

        case 17:
            sendGetRequest();
            break;

        case 22:
            sendPOSTRequest_partialUpdate_userInput();
            break;

        case 25:
            observe_request();
            break;

        case 26:
            stop_observe();
            break;

        case 31:
            printf("\nPlease enter the group URI\n");
            /* 'count' specifies the number of bytes allocated to 'char* collection_uri_input'
            using malloc */
            unsigned int count = 25;
            char* collection_uri_input = malloc((size_t)count);
            int scanf_returnValue = scanf("%s", collection_uri_input);
            if(scanf_returnValue == 1) {
               findGroup(collection_uri_input);
            }
            else
               printf("Failed to read URI");
            break;

        case 33:
            printf("Update Group option chosen\n");
            printf("\nEnter the URI of group to be updated\n");
            /* 'count2' specifies the number of bytes allocated to 'char* collection_uri_input2'
            using malloc */
            unsigned int count2 = 25;
            char* collection_uri_input2 = malloc((size_t)count2);
            int scanf_returnValue2 = scanf("%s", collection_uri_input2);
            if(scanf_returnValue2 == 1){
               collectionPointer = updateGroup(collection_uri_input2);
            }
            else
               printf("Failed to read URI");
            break;

        case 34:
            printf("'Update local resource maually' chosen\n");
            printf("Please enter the URI of resource to be updated manually\n");
            unsigned int count3 = 25;
            char* resource_uri_input = malloc((size_t)count3);
            int scanf_returnValue3 = scanf("%s", resource_uri_input);
            if(scanf_returnValue3 == 1){
               printf("\nResource URI read is as follows: %s\n", resource_uri_input);
               if(g_isTempResourceCreated == true)
               updateLocalResourceManually(resource_uri_input);
               else
               printf("\n!!!!!Please create resource first!!!!!\n");
            }
            else
               printf("Failed to read URI");
            break;

        case 107:
            createSingleAirConResource();
            break;

        case 0:
            oc_main_shutdown();
            exit(0);
            break;

        default:
            printf("Invalid Input. Please input your choice again\n");
    }
}

static void
get_light(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
     printf("\n get_light called!!\n");
    (void)user_data;
    (void)request;
    (void)interface;

}

/*
calling the register_resources  function of device_builder_server.c
*/
static void createResource()
{
    printf("\ncreateResource called!!\n");

    if (g_isTempResourceCreated == false) {
        register_resources();
        printf("Resource created successfully\n");
        g_isTempResourceCreated = true;
    }
    else {
        printf("Resource already created\n");
    }
}

/*
   passing the invisible resource uri
*/
static void createInvisibleResource()
{
    printf("createInvisibleResource called!!\n");

    if (g_isInvisibleResourceCreated == false) {
        oc_resource_t *res = oc_new_resource(NULL, FAN_INVISIBLE_URI, 1, 0);
        oc_resource_set_default_interface(res, OC_IF_RW);
        oc_resource_set_discoverable(res, true);
        oc_resource_set_periodic_observable(res, 1);
        oc_resource_set_request_handler(res, OC_GET, get_light, NULL);
        g_addInvisibleResource = oc_add_resource(res);

        if (g_addInvisibleResource == true) {
            printf("Invisible Light Resource created successfully\n");
            g_isInvisibleResourceCreated = true;
        }
        else {
            printf("Unable to create Invisible Light Resource \n");
        }
    }
    else {
          printf("Resource already created!!\n");
    }
}

/*creating the the resource by using url*/
static void createResourceWithUrl()
{
    printf("Creating Resource with complete URL called!!\n");
    if (g_createResourceWithURL == false) {
        oc_resource_t *res = oc_ri_get_app_resource_by_uri(g_binaryswitch_RESOURCE_URI, strlen(g_binaryswitch_RESOURCE_URI), 0);
        if (res->uri.ptr == g_binaryswitch_RESOURCE_URI) {
            printf("Creating Resource with complete URL already created\n");
            g_createResourceWithURL = true;
        }
    }
    else {
        printf("Resource with complete URL already created!!\n");
    }
}

oc_discovery_flags_t discovery(const char *di, const char *uri,
        oc_string_array_t types, oc_interface_mask_t interfaces,
        oc_endpoint_t *endpoint, oc_resource_properties_t bm, void *user_data) {
    (void) di;
    (void) interfaces;
    (void) user_data;
    (void) bm;
    int i;
    int uri_len = strlen(uri);
    uri_len = (uri_len >= MAX_URI_LENGTH) ? MAX_URI_LENGTH - 1 : uri_len;
    PRINT("\ndiscovery: %s", uri);

    for (i = 0; i < (int) oc_string_array_get_allocated_size(types); i++) {
        char *t = oc_string_array_get_item(types, i);

        if (strlen(t) == 19 && strncmp(t, "oic.r.switch.binary", 19) == 0) {

            light_server = endpoint;
            strncpy(a_light, uri, uri_len);
            a_light[uri_len] = '\0';
            oc_endpoint_t *ep = endpoint;
            while (ep != NULL) {
                PRINT("\nIP address: \n");
                PRINTIPaddr(*ep);
                PRINT("\nPort:\n");
                PRINTport(*ep);
                PRINT("\n");
                ep = ep->next;
            }
            resource_found =true;
            PRINT("Resource %s hosted at endpoints:\n", a_light);

           return OC_STOP_DISCOVERY;

        }
    }
    oc_free_server_endpoints(endpoint);
        fflush(stdout);
    return OC_CONTINUE_DISCOVERY;
}
static void findAllResources()
{
    printf("Find All Resources called");
    fflush(stdout);
    oc_do_ip_discovery(NULL, &discovery, NULL);

}
static oc_discovery_flags_t
discovery_ResourceType(const char *anchor, const char *uri, oc_string_array_t types,
          oc_interface_mask_t interfaces, oc_endpoint_t *endpoint,
          oc_resource_properties_t bm, void *user_data)
{

  (void)anchor;
  (void)user_data;
  (void)interfaces;
  (void)bm;
  int i;
  int uri_len = strlen(uri);
  uri_len = (uri_len >= MAX_URI_LENGTH) ? MAX_URI_LENGTH - 1 : uri_len;
  for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
    char *t = oc_string_array_get_item(types, i);
    if (strcmp(UserResourceType_input, t) == 1) {
      printf("Inside the userResourceType ");
      resource_server = endpoint;
      strncpy(a_light, uri, uri_len);
      a_light[uri_len] = '\0';

      PRINT("Resource %s hosted at endpoints:\n", a_light);

      oc_endpoint_t *ep = endpoint;
      while (ep != NULL) {
        PRINTipaddr(*ep);
        PRINT("\n");
        ep = ep->next;
      }
      printf("OC_STOP_DISCOVERY retuened");

   return OC_STOP_DISCOVERY;
    }
  }
  oc_free_server_endpoints(endpoint);
  return OC_CONTINUE_DISCOVERY;
}

void findResource_UserResType(char *userResourceType)
{
    printf("\nfindResource_UserResType called, user input is %s\n", userResourceType);
     bool discoverResourcetype = false;
    if (userResourceType!=NULL) {

        printf("call discovery!!\n");
        fflush(stdout);
        discoverResourcetype = oc_do_ip_discovery(userResourceType, &discovery_ResourceType, NULL );

    }

    if (discoverResourcetype == true) {
        printf("userResourceType Discovered\n");
    }
    else {
        printf("userResourceType is not Discovered");
    }
}
static bool
is_resource_found(void)
{
  if (!resource_found) {
    printf("Please discovery resource first!\n");
    return false;
  }

  return true;
}

static void
get_response(oc_client_response_t *data)
{
  printf("GET_light:\n");

  (void)data;

}

static void sendGetRequest()
{

  if (!is_resource_found())
    return;

  oc_do_get(a_light, light_server, NULL, &get_response, LOW_QOS, NULL);
  printf("SEND GET request is called");

}

static void
observe_response(oc_client_response_t *data)
{

  (void)data;
  printf("OBSERVE_light:\n");

}

static void
observe_request(void)
{
  if (!is_resource_found())
    return;

  oc_do_observe(a_light, light_server, NULL, &observe_response, LOW_QOS, NULL);
  printf("Sent OBSERVE request\n");
}

static void
stop_observe(void)
{
  if (!is_resource_found())
    return;

  printf("Stopping OBSERVE\n");
  if (!oc_stop_observe(a_light, light_server)) {
    printf("Please observe start first!\n");
  }
}

static oc_discovery_flags_t
discovery_Introspection(const char *anchor, const char *uri, oc_string_array_t types,
          oc_interface_mask_t interfaces, oc_endpoint_t *endpoint,
          oc_resource_properties_t bm, void *user_data)
{
  (void)anchor;
  (void)user_data;
  (void)interfaces;
  (void)bm;
  int i;
     PRINT("test:\n");
  int uri_len = strlen(uri);
  uri_len = (uri_len >= MAX_URI_LENGTH) ? MAX_URI_LENGTH - 1 : uri_len;
  for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
    char *t = oc_string_array_get_item(types, i);
     printf("discoverIntrospection option Called %s ", t);
    if (strcmp("oic.wk.introspection", t) == 0) {
      resource_server = endpoint;
      strncpy(a_light, uri, uri_len);
      a_light[uri_len] = '\0';

      PRINT("Resource %s hosted at endpoints:\n", a_light);
      s_generalQuit = 1;
      oc_endpoint_t *ep = endpoint;
      while (ep != NULL) {
        PRINTipaddr(*ep);
        PRINT("\n");
        ep = ep->next;
      }
 
   return OC_STOP_DISCOVERY;
    }
  }
  oc_free_server_endpoints(endpoint);
  return OC_CONTINUE_DISCOVERY;
}

void discoverIntrospection()
{
     printf("Discovering Introspection using Multicast... ");
     oc_do_ip_discovery("oic.wk.introspection", &discovery_Introspection, NULL);

}

/*
input is baseUri,lightCount added in the baseUri
*/
static void createManyLightResources()
{
    printf("createManyLightResources called!!\n");

    bool  add_LightResource;
    char baseUri[20] = "/device/light-";
    int lightCount = LIGHT_COUNT;
    char uri[20] = "";

    if (g_isManyLightCreated == false) {

        for (int i = 0; i < MAX_LIGHT_RESOURCE_COUNT; i++, lightCount++) {
            sprintf(uri,"/device/light-%d", lightCount);
            printf("%s\n",baseUri);
            printf("%s\n",uri);

            oc_resource_t *res = oc_new_resource(RESOURCE_NAME, uri, NUM_RESOURCES_TYPES, NUM_DEVICE);
            oc_resource_bind_resource_type(res, RESOURCE_LIGHT_TYPE);
            oc_resource_bind_resource_interface(res, OC_IF_BASELINE);
            oc_resource_set_default_interface(res, OC_IF_RW);
            oc_resource_set_discoverable(res, true);
            oc_resource_set_periodic_observable(res, 1);
            oc_resource_set_request_handler(res, OC_GET, get_light, NULL);
            add_LightResource = oc_add_resource(res);

            if (add_LightResource == true) {

                printf("Light Resource created successfully with uri:\n");
                g_isManyLightCreated = true;
            }
            else {
                printf("Unable to create Light resource with uri\n");
            }
        }
    }
    else {
      printf("Many Light Resources already created!!\n");
    }
}

static oc_collection_t* createGroupResource()
{

    oc_resource_t *resource_one = oc_ri_get_app_resource_by_uri(RESOURCE_1_URI, strlen(RESOURCE_1_URI),0);
    oc_resource_t *resource_two = oc_ri_get_app_resource_by_uri(RESOURCE_2_URI, strlen(RESOURCE_2_URI),0);

    const char* collection_name = "Example Collection";
    const char* collection_uri = "/example_collection_path";

   oc_resource_t *new_collection = oc_new_collection(collection_name, collection_uri, 2, 0);

    #if defined(OC_COLLECTIONS)

        if(new_collection != NULL)
            printf("New collection created with the following name: %s\n",new_collection->name.ptr);

        oc_resource_bind_resource_type(new_collection, "oic.wk.col");
        oc_resource_set_discoverable(new_collection, true);

        oc_link_t *link_one = oc_new_link(resource_one);
        oc_collection_add_link(new_collection, link_one);

        oc_link_t *link_two = oc_new_link(resource_two);
        oc_collection_add_link(new_collection, link_two);

        oc_add_collection(new_collection);

    #endif /* OC_COLLECTIONS */

    return (oc_collection_t*)new_collection;

}

static void deleteAllResources()
{
    printf("deteAllResources called!!\n");

    oc_resource_t *res = oc_ri_get_app_resources();

    while(res)
    {
        oc_ri_delete_resource(res);
        res = oc_ri_get_app_resources();
    }

    if (res == NULL) {
        printf("All Resources Deleted\n");
    }

    g_isManyLightCreated = false;
    g_addInvisibleResource = false;
    g_isTempResourceCreated = false;
    g_createResourceWithURL = false;
    g_isAirConDeviceCreated = false;
}

static void deleteCreatedGroup()
{

    oc_collection_free(collectionPointer);
    printf("\nCollection deleted\n");
}

static void
post_response(oc_client_response_t *data)
{
  printf("POST_light:\n");
  if (data->code == OC_STATUS_CHANGED)
    printf("POST response: CHANGED\n");
  else if (data->code == OC_STATUS_CREATED)
    printf("POST response: CREATED\n");
  else
    printf("POST response code %d\n", data->code);
}

static void sendPOSTRequest_partialUpdate_userInput()
{
  if (!is_resource_found())
    return;

  printf("\nPlease Input attribute key(string)\n");
  unsigned int count = 15;
  char* attribute_key = malloc((size_t)count);
  int scanf_return_value1 = scanf("%s", attribute_key);
  if(scanf_return_value1)
      printf("Attribute key entered is '%s'\n", attribute_key);

  printf("\nPlease Input attribute value(integer)\n");
  int attribute_value;
  int scanf_return_value2 = scanf("%d",&attribute_value);
  if(scanf_return_value2)
      printf("Attribute value entered is '%d'\n", attribute_value);

  if (oc_init_post(a_light, light_server, NULL, &post_response, LOW_QOS, NULL)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, false);
    oc_rep_set_int(root, attribute_key, attribute_value);
    oc_rep_end_root_object();
    if (oc_do_post())
      printf("\nSent POST request\n");
    else
      printf("\nCould not send POST request\n");
  } else
    printf("\nCould not init POST request\n");
}

static oc_collection_t* updateGroup(char* uri_input)
{
    oc_collection_t* tempCollection = oc_get_collection_by_uri(uri_input, strlen(uri_input), 0);
    int optionNumber;

    if(tempCollection == NULL){
        printf("!!Collection not found!! Collection URI entered may be incorrect OR collection is not created OR both\n");
    }
    else{

    printf("\nResources part of '%s' are as follows:\n", tempCollection->name.ptr);

    oc_link_t* resource_links = oc_collection_get_links((oc_resource_t*)tempCollection);
    int count_resource=1;

    while(resource_links != NULL){
        printf("Resource %d - URI: '%s'\n", count_resource, resource_links->resource->uri.ptr);
        count_resource++;
        resource_links = resource_links->next;
    }

    printf("..................\n");
    printf("Update Group Menu:\n");
    printf("..................\n");
    printf("Choose from below options by entering the option number\n1. Update collection properties\n"
        "2. Add a resource to group\n3. Remove a resource from group\n4. Update a resource's properties\n");

    int scanf_return_value = scanf("%d",&optionNumber);
    if(scanf_return_value)
        printf("Option chosen:%d\n",optionNumber);

    switch(optionNumber) {

        case 1:   ;
            int collectionPropertyOptionNumber = -1;

            unsigned int count_case1 = 25;
            char* resource_property_case1 = malloc((size_t)count_case1);

            int collectionInterfaceIntegerValue = 1;

            while(collectionPropertyOptionNumber != 0)
            {
            case1_label:
                printf("\t\t..................\n");
                printf("\t\tUpdate collection properties:\n");
                printf("\t\t..................\n");
                printf("\t\tChoose from below options by entering the option number\n\t\t1. Update collection type\n"
                "\t\t2. Update collection interface\n\t\t3. Update collection path\n\t\t0. Exit Update collection Properties\n");

                int scanf_returnValue_case1 = scanf("%d",&collectionPropertyOptionNumber);
                if(scanf_returnValue_case1)
                {
                    if(collectionPropertyOptionNumber == 0)
                    {
                        printf("'Exit Update Collection Properties' chosen\n");
                        break;
                    }
                    else if(collectionPropertyOptionNumber == 1)
                    {
                        printf("Option 1 chosen.\n");
                        printf("Presently, collection type is '%s'\n",tempCollection->types.ptr);
                        printf("Please enter new 'collection type'\n");
                        int scanf_returnValue_case1_2 = scanf("%s", resource_property_case1);
                        if(scanf_returnValue_case1_2)
                        printf("\nCollection property read is as follows: %s\n", resource_property_case1);
                    }
                    else if(collectionPropertyOptionNumber == 2)
                    {
                        printf("\nOption 2 chosen.\n");
                        printf("Presently, interface value is %d\n\n",tempCollection->interfaces);
                        printf("Please enter a suitable integer number using the guide below\nto have"
                            "the required interfaces for the collection\n\n");
                        printf("2: corresponds to Baseline(OC_IF_BASELINE) interface\n"
                            "4: corresponds to Link Lists(OC_IF_LL) interface\n"
                            "8: corresponds to Batch(OC_IF_B) interface\n"
                            "16: corresponds to Read-only(OC_IF_R) interface\n"
                            "32: corresponds to Read-Write(OC_IF_RW) interface\n"
                            "64: corresponds to Actuator(OC_IF_A) interface\n"
                            "128: corresponds to Sensor(OC_IF_S) interface\n");
                        printf("66(2+64): corresponds to interfaces of Baseline(2) and Actuator(64)\n");
                        printf("18(2+16): corresponds to interfaces of Baseline(2) and Read-only(16)\netc.,\n");
                        int scanf_returnValue_case1_3 = scanf("%d",&collectionInterfaceIntegerValue);
                        if (scanf_returnValue_case1_3)
                            printf("Collection Interface Integer Value entered: %d\n", collectionInterfaceIntegerValue);

                    }
                    else if (collectionPropertyOptionNumber == 3)
                    {
                        printf("Option 3 chosen.\n");
                        printf("Presently, collection URI is '%s'\n",tempCollection->uri.ptr);
                        printf("Please enter 'new collection URI'\n");
                        int scanf_returnValue_case1_4 = scanf("%s", resource_property_case1);
                        if(scanf_returnValue_case1_4)
                        printf("\nNew URI read is as follows: %s\n", resource_property_case1);
                    }
                    else
                    {
                        printf("!!!!!!!!Invalid option chosen!!!!!!!!\n");
                        goto case1_label;
                    }
                }
                else
                    printf("Read from scanf was unsuccessful\n");

                switch(collectionPropertyOptionNumber)
                {
                    case 1:
                        printf("\nResource type of collection '%s' before updation: %s\n", tempCollection->name.ptr,
                        tempCollection->types.ptr);
                        tempCollection->types.ptr = resource_property_case1;
                        printf("Resource type of collection '%s' after updation: %s\n", tempCollection->name.ptr,
                        tempCollection->types.ptr);
                    break;

                    case 2:
                        printf("\nInterface of collection '%s' before updation: %d\n", tempCollection->name.ptr,
                             tempCollection->interfaces);

                        tempCollection->interfaces = collectionInterfaceIntegerValue;

                        printf("Interface of collection '%s' after updation: %d\n", tempCollection->name.ptr,
                        tempCollection->interfaces);
                    break;

                    case 3:
                        /* Still working on updating collection path */
                        printf("\nURI of collection '%s' before updation: %s\n", tempCollection->name.ptr,
                        tempCollection->uri.ptr);
                        tempCollection->uri.ptr = resource_property_case1;
                        printf("URI of collection '%s' after updation: %s\n", tempCollection->name.ptr,
                        tempCollection->uri.ptr);
                    break;

                    default: printf("Entered 'default case' of switch statement in 'case 4' of outer switch statement\n");
                    break;
                }
            }
        break;

        case 2: printf("\nEnter the URI of the resource to be added to the group '%s'\n", tempCollection->name.ptr);
            unsigned int count_case2 = 25;
            char* resource_uri_case2 = malloc((size_t)count_case2);
            int scanf_returnValue_case2 = scanf("%s", resource_uri_case2);

            if(scanf_returnValue_case2)
                printf("");

            oc_resource_t *resource_case2 = oc_ri_get_app_resource_by_uri(resource_uri_case2, strlen(resource_uri_case2),0);

            if(resource_case2 != NULL)
            {
                oc_link_t *link_one = oc_new_link(resource_case2);
                oc_collection_add_link((oc_resource_t*)tempCollection, link_one);
                printf("\nLink of resource with URI:'%s' has been added to the group '%s'\n", resource_uri_case2, tempCollection->name.ptr);
            }
            else{
                printf("\n!!!!!Resource with above entered URI is not found!!!!!\n");
            }

            printf("\nFollowing resources are part of '%s' after addition of resource\n", tempCollection->name.ptr);

            oc_link_t* resource_links_case2 = oc_collection_get_links((oc_resource_t*)tempCollection);
            int count_case2_1=1;

            while(resource_links_case2 != NULL)
            {
                printf("Resource %d - URI: '%s'\n", count_case2_1, resource_links_case2->resource->uri.ptr);
                count_case2_1++;
                resource_links_case2 = resource_links_case2->next;
            }

            break;

        case 3: printf("\nEnter the URI of the resource to be removed from the group\n");
            unsigned int count_case3 = 25;
            char* resource_uri_case3 = malloc((size_t)count_case3);
            int scanf_returnValue_case3 = scanf("%s", resource_uri_case3);
            if(scanf_returnValue_case3)
                printf("\nResource URI read is as follows: %s\n", resource_uri_case3);

            oc_resource_t *resource_case3 = oc_ri_get_app_resource_by_uri(resource_uri_case3, strlen(resource_uri_case3),0);
            if(resource_case3 == NULL)
            printf("\n!!!!!Resource with above entered URI is not found!!!!!\n");
            else{
            oc_link_t* resource_links_case3 = oc_collection_get_links((oc_resource_t*)tempCollection);

            while(resource_links_case3 != NULL)
            {
                if(!strcmp(resource_links_case3->resource->uri.ptr, resource_uri_case3))
                {
                    printf("\nLink of resource with uri: '%s' will be removed from '%s'\n", resource_links_case3->resource->uri.ptr,
                        tempCollection->name.ptr);
                    oc_collection_remove_link((oc_resource_t*)tempCollection, resource_links_case3);
                    printf("\n........Resource(resource link) removed from the group........\n");
                    break;
                }
                resource_links_case3 = resource_links_case3->next;
            }
            }
            break;

        case 4: printf("Enter the URI of resource whose properties are to be updated\n");
            int resourcePropertyOptionNumber = -1;

            unsigned int count_case4 = 25;
            char* resource_uri_case4 = malloc((size_t)count_case4);
            int scanf_returnValue_case4 = scanf("%s", resource_uri_case4);
            if(scanf_returnValue_case4)
            printf("\nResource URI read is as follows: %s\n", resource_uri_case4);

            oc_resource_t *resource_case4 = oc_ri_get_app_resource_by_uri(resource_uri_case4, strlen(resource_uri_case4),0);

            if(resource_case4 == NULL){
                printf("!!!!!Resource not found!!!!!\nPlease choose 'Option 33', 'sub-option 4' again and enter approriate URI\n");
            }
            else{

            unsigned int count_case4_2 = 25;
            char* resource_property_case4 = malloc((size_t)count_case4_2);

            int resourceInterfaceIntegerValue = 1;

            while(resourcePropertyOptionNumber != 0)
            {
            case4_label:
                printf("\t\t..................\n");
                printf("\t\tUpdate resource properties:\n");
                printf("\t\t..................\n");
                printf("\t\tChoose from below options by entering the option number\n\t\t1. Update resource type\n"
                "\t\t2. Update resource interface\n\t\t3. Update resource path\n\t\t0. Exit Update Resource Properties\n");

                int scanf_returnValue_case4_2 = scanf("%d",&resourcePropertyOptionNumber);
                if(scanf_returnValue_case4_2)
                {
                    if(resourcePropertyOptionNumber == 0)
                    {
                        printf("'Exit Update Resource Properties' chosen\n");
                        break;
                    }
                    else if(resourcePropertyOptionNumber == 1)
                    {
                        printf("Option 1 chosen.\n");
                        printf("Presently, resource type is '%s'\n",resource_case4->types.ptr);
                        printf("Please enter new 'resource type'\n");
                        int scanf_returnValue_case4_3 = scanf("%s", resource_property_case4);
                        if(scanf_returnValue_case4_3)
                        printf("\nResource property read is as follows: %s\n", resource_property_case4);
                    }
                    else if(resourcePropertyOptionNumber == 2)
                    {
                        printf("\nOption 2 chosen.\n");
                        printf("Presently, interface value is %d\n\n",resource_case4->interfaces);
                        printf("Please enter a suitable integer number using the guide below\nto have the required interfaces for the resource\n\n");
                        printf("2: corresponds to Baseline(OC_IF_BASELINE) interface\n"
                            "4: corresponds to Link Lists(OC_IF_LL) interface\n"
                            "8: corresponds to Batch(OC_IF_B) interface\n"
                            "16: corresponds to Read-only(OC_IF_R) interface\n"
                            "32: corresponds to Read-Write(OC_IF_RW) interface\n"
                            "64: corresponds to Actuator(OC_IF_A) interface\n"
                            "128: corresponds to Sensor(OC_IF_S) interface\n");
                        printf("66(2+64): corresponds to interfaces of Baseline(2) and Actuator(64)\n");
                        printf("18(2+16): corresponds to interfaces of Baseline(2) and Read-only(16)\netc.,\n");
                        int scanf_returnValue_case4_4 = scanf("%d",&resourceInterfaceIntegerValue);
                        if (scanf_returnValue_case4_4)
                            printf("Resource Interface Integer Value entered: %d\n", resourceInterfaceIntegerValue);

                    }
                    else if (resourcePropertyOptionNumber == 3)
                    {
                        printf("Option 3 chosen.\n");
                        printf("Presently, resource uri is '%s'\n",resource_case4->uri.ptr);
                        printf("Please enter new 'resource URI'\n");
                        int scanf_returnValue_case4_5 = scanf("%s", resource_property_case4);
                        if(scanf_returnValue_case4_5)
                        printf("\nNew URI read is as follows: %s\n", resource_property_case4);
                    }
                    else
                    {
                        printf("!!!!!!!!Invalid option chosen!!!!!!!!\n");
                        goto case4_label;
                    }
                }
                else
                    printf("Read from scanf was unsuccessful\n");

                switch(resourcePropertyOptionNumber)
                {
                    case 1: 
                        printf("\nResource type of resource '%s' before updation: %s\n",resource_case4->name.ptr,resource_case4->types.ptr);
                        resource_case4->types.ptr = resource_property_case4;
                        printf("Resource type of resource '%s' after updation: %s\n", resource_case4->name.ptr,
                        resource_case4->types.ptr);
                        break;

                    case 2:
                        printf("\nResource interface of resource '%s' before updation: %d\n", resource_case4->name.ptr,
                        resource_case4->interfaces);

                        resource_case4->interfaces = resourceInterfaceIntegerValue;

                        printf("Resource interface of resource '%s' after updation: %d\n", resource_case4->name.ptr,
                        resource_case4->interfaces);
                        break;

                    case 3:
                        printf("\nResource URI of resource '%s' before updation: %s\n", resource_case4->name.ptr, 
                        resource_case4->uri.ptr);
                        resource_case4->uri.ptr = resource_property_case4;
                        printf("Resource URI of resource '%s' after updation: %s\n", resource_case4->name.ptr,
                        resource_case4->uri.ptr);
                        break;

                    default: printf("Entered 'default case' of switch statement in 'case 4' of outer switch statement\n");
                        break;
                }
            }
            }
            break;

        default: printf("!!!!Invalid Option Entered!!!!\n");
            break;
    }

    }

    return tempCollection;
}

static void updateLocalResourceManually(char* uri_input)
{
    oc_resource_t *resource = oc_ri_get_app_resource_by_uri(uri_input, strlen(uri_input),0);
    if(resource == NULL){
        printf("\n!!!!!Resource not found!!!!!\nPlease choose 'Option 34' again and enter approriate URI\n");
        return;
    }
    else{
    unsigned int count_case4_2 = 25;
    char* resource_property_case4 = malloc((size_t)count_case4_2);

    int resourcePropertyOptionNumber = -1;
    int resourceInterfaceIntegerValue = 1;
            while(resourcePropertyOptionNumber != 0)
            {
            case4_label:
                printf("\t\t..................\n");
                printf("\t\tUpdate resource properties:\n");
                printf("\t\t..................\n");
                printf("\t\tChoose from below options by entering the option number\n\t\t1. Update resource type\n"
                       "\t\t2. Update resource interface\n\t\t3. Update resource path\n\t\t0. Exit Update Resource Properties\n");

                int scanf_returnValue_case4_2 = scanf("%d",&resourcePropertyOptionNumber);
                if(scanf_returnValue_case4_2)
                {
                    if(resourcePropertyOptionNumber == 0)
                    {
                        printf("'Exit Update Resource Properties' chosen\n");
                        break;
                    }
                    else if(resourcePropertyOptionNumber == 1)
                    {
                        printf("Option 1 chosen.\n");
                        printf("Presently, resource type is '%s'\n",resource->types.ptr);
                        printf("Please enter new 'resource type'\n");
                        int scanf_returnValue_case4_3 = scanf("%s", resource_property_case4);
                        if(scanf_returnValue_case4_3)
                        printf("\nResource property read is as follows: %s\n", resource_property_case4);
                    }
                    else if(resourcePropertyOptionNumber == 2)
                    {
                        printf("\nOption 2 chosen.\n");
                        printf("Presently, interface value is %d\n\n",resource->interfaces);
                        printf("Please enter a suitable integer number using the guide below\nto have the required interfaces for the resource\n\n");
                        printf("2: corresponds to Baseline(OC_IF_BASELINE) interface\n"
                            "4: corresponds to Link Lists(OC_IF_LL) interface\n"
                            "8: corresponds to Batch(OC_IF_B) interface\n"
                            "16: corresponds to Read-only(OC_IF_R) interface\n"
                            "32: corresponds to Read-Write(OC_IF_RW) interface\n"
                            "64: corresponds to Actuator(OC_IF_A) interface\n"
                            "128: corresponds to Sensor(OC_IF_S) interface\n");
                        printf("66(2+64): corresponds to interfaces of Baseline(2) and Actuator(64)\n");
                        printf("18(2+16): corresponds to interfaces of Baseline(2) and Read-only(16)\netc.,\n");
                        int scanf_returnValue_case4_4 = scanf("%d",&resourceInterfaceIntegerValue);
                        if (scanf_returnValue_case4_4)
                            printf("Resource Interface Integer Value entered: %d\n", resourceInterfaceIntegerValue);

                    }
                    else if (resourcePropertyOptionNumber == 3)
                    {
                        printf("Option 3 chosen.\n");
                        printf("Presently, resource uri is '%s'\n",resource->uri.ptr);
                        printf("Please enter new 'resource URI'\n");
                        int scanf_returnValue_case4_5 = scanf("%s", resource_property_case4);
                        if(scanf_returnValue_case4_5)
                        printf("\nNew URI read is as follows: %s\n", resource_property_case4);
                    }
                    else
                    {
                        printf("!!!!!!!!Invalid option chosen!!!!!!!!\n");
                        goto case4_label;
                    }
                }
                else
                    printf("Read from scanf was unsuccessful\n");

                switch(resourcePropertyOptionNumber)
                {
                    case 1: printf("\nResource type before updation: %s\n",resource->types.ptr);
                        resource->types.ptr = resource_property_case4;
                        printf("Resource type after updation: %s\n",resource->types.ptr);
                        break;

                    case 2:
                        printf("\nResource interface before updation: %d\n",resource->interfaces);
                        resource->interfaces = resourceInterfaceIntegerValue;
                        printf("Resource interface after updation: %d\n",resource->interfaces);
                        break;

                    case 3:printf("\nResource URI before updation: %s\n",resource->uri.ptr);
                        resource->uri.ptr = resource_property_case4;
                        printf("Resource URI after updation: %s\n",resource->uri.ptr);
                        break;

                    default: printf("Entered 'default case' of switch statement\n");
                        break;
                }
            }
    }
}

static void findGroup(char* uri_input)
{

    printf("\nFind Group option chosen\n");

    printf("\nparameter passed: %s\n",uri_input);

    oc_collection_t* tempCollection_findGroup = oc_get_collection_by_uri(uri_input, strlen(uri_input), 0);

    if(tempCollection_findGroup == NULL)
        printf("\nGroup does not exist\n");
    else
        printf("\nGroup found. Collection name is %s\n\n", tempCollection_findGroup->name.ptr);

}



void
get_binaryswitchcb(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
    (void)user_data;  // not used

    printf("get_binaryswitch: interface %d\n", interfaces);
    oc_rep_start_root_object();
    switch (interfaces) {
    case OC_IF_BASELINE:
    /* fall through */
    case OC_IF_A:
        printf("Adding Baseline info\n" );
        oc_process_baseline_interface(request->resource);
        oc_rep_set_boolean(root, value, g_binaryswitch_valuecb);
        printf("   %s : %d\n", g_binaryswitch_RESOURCE_PROPERTY_NAME_value,  g_binaryswitch_valuecb );
        break;
        default:
        break;
    }
    oc_rep_end_root_object();
    oc_send_response(request, OC_STATUS_OK);
}

void
post_binaryswitchcb(oc_request_t *request, oc_interface_mask_t interfaces, void *user_data)
{
    (void)interfaces;
    (void)user_data;
    bool error_state = false;
    printf("post_binaryswitch:\n");
    oc_rep_t *rep = request->request_payload;
    while (rep != NULL) {
        printf("key: (check) %s ", oc_string(rep->name));
        if (strcmp ( oc_string(rep->name), g_binaryswitch_RESOURCE_PROPERTY_NAME_value) == 0) {
      // value exist in payload

            if (rep->type != OC_REP_BOOL)
                {
                    error_state = true;
                    printf ("   property 'value' is not of type bool %d \n", rep->type);
            }
        }
                rep = rep->next;
    }
           if (error_state == false) {
              oc_rep_t *rep = request->request_payload;
              while (rep != NULL) {
                    printf("key: (assign) %s ", oc_string(rep->name));
                   // no error: assign the variables
                    if (strcmp ( oc_string(rep->name), g_binaryswitch_RESOURCE_PROPERTY_NAME_value)== 0){
                      // assign value
                       g_binaryswitch_valuecb = rep->value.boolean;
                    }
                       rep = rep->next;
            }
                      // set the response
                      oc_rep_start_root_object();
                      oc_rep_set_boolean(root, value, g_binaryswitch_valuecb);
                      oc_rep_end_root_object();
                      oc_send_response(request, OC_STATUS_CHANGED);
            }
          else {
              // TODO: add error response, if any
              oc_send_response(request, OC_STATUS_NOT_MODIFIED);
          }
}

int  app_init1()
 {

    int  ret = oc_init_platform(ENGLISH_NAME_VALUE, NULL, NULL);
    ret |= oc_add_device("oic/d", "oic.d.airconditioner", "AirConditioner",
                             OCF_SPEC_VERSION, OCF_DATA_MODEL_VERSION,
                             NULL, NULL);
    return ret;
}


/*using the  uri creating the air conditioner resource*/
static void createSingleAirConResource()
{
    printf("Creating AirCon Device Resources!!\n");

    if (g_isAirConDeviceCreated == false) {

        int init;
        static const oc_handler_t handler = {.init = app_init1,
                                       .signal_event_loop = signal_event_loop,
                                       };

        init = oc_main_init(&handler);

        if (init < 0)
            printf("Not Able to Intialize the mainHandler");


       oc_resource_t *res = oc_new_resource("AC-binaryswitch", RESOURCE_AIR_URI, RESOURCE_INTERFACE, DEVICE_COUNT);
       oc_resource_bind_resource_type(res,SWITCH_RESOURCE_TYPE);
       for ( int a = 0; a < RESOURCE_INTERFACE; a++ )
            {
                oc_resource_bind_resource_interface(res, convert_if_string(g_binaryswitch_AIRCON_RESOURCE_INTERFACE[a]));
            }
       oc_resource_set_discoverable(res, true);
       oc_resource_set_periodic_observable(res, OBSERVE_PERIODIC);
       oc_resource_set_request_handler(res, OC_GET, get_binaryswitchcb, NULL);
       oc_resource_set_request_handler(res, OC_POST, post_binaryswitchcb, NULL);
       bool add_res = oc_add_resource(res);

       if (add_res == true) {
           printf("AirCon Binary Switch Resource created successfully\n");
           g_isAirConDeviceCreated = true;
       }
       else {
            printf("Unable to create AirCon Binary Switch resource\n");
       }
    }
    else
    {
        printf("Already Smart Home Air Conditioner Device Resource is created!!\n");
    }
}


static void *
process_func(void *data)
{
  (void)data;
  oc_clock_time_t next_event;

  while (quit != 1) {
    pthread_mutex_lock(&app_mutex);
    next_event = oc_main_poll();
    pthread_mutex_unlock(&app_mutex);
    pthread_mutex_lock(&mutex);
    if (next_event == 0) {
        //printf("\nwaiting at process_func \n");
        fflush(stdout);
        pthread_cond_wait(&cv, &mutex);
    }
    else {
        ts.tv_sec = (next_event / OC_CLOCK_SECOND);
        ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
        pthread_cond_timedwait(&cv, &mutex, &ts);
    }
    pthread_mutex_unlock(&mutex);
  }

  pthread_exit(0);
}

int main()
{
    struct sigaction sa;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    oc_set_con_res_announced(false);

    int init;
    static const oc_handler_t handler = {.init = app_init,
                                       .signal_event_loop = signal_event_loop
                                      // .register_resources = register_resources
    #ifdef OC_CLIENT
                                       ,
                                       .requests_entry = 0
    #endif
                                       };


    #ifdef OC_SECURITY
        oc_storage_config("./CertificationAppServer_creds/");
    #endif /* OC_SECURITY */

  if (pthread_mutex_init(&mutex, NULL) < 0) {
    printf("pthread_mutex_init failed!\n");
    return -1;
  }

  if (pthread_mutex_init(&app_mutex, NULL) < 0) {
    printf("pthread_mutex_init failed!\n");
    pthread_mutex_destroy(&mutex);
    return -1;
  }

    init = oc_main_init(&handler);
    if (init < 0)
        return init;
                    pthread_t thread;
    if (pthread_create(&thread, NULL, process_func, NULL) != 0) {
       printf("Failed to create main thread\n");
       init = -1;
       goto exit;
    }

    while (quit != 1) {
       showMenu(0, NULL);

       /* Take the input from user and do the selected operation*/
       handleMenu();
    }
    exit:
      pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&app_mutex);
    oc_main_shutdown();
    return 0;
}

void handleMenu()
{
    int choice;
    do {
           if (scanf("%d", &choice)) {
               printf("\n");
               if (!quit) {
                  selectMenu(choice);
                  showMenu(0, NULL);
               }
            }
    } while(choice && (quit == 0));
}
