// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h> //printf
#include <string.h> //strcmp
#include <stdlib.h> //atoi

#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU * STREAM_HISTORY

int main(int args, char** argv)
{
    // CLI
    if(3 > args || 0 == atoi(argv[2]))
    {
        printf("usage: program [-h | --help] | ip port topic_size]\n");
        return 0;
    }

    char* ip = argv[1];
    char* port = argv[2];
    uint32_t topic_size = (uint32_t)atoi(argv[3]);
    uint32_t buffer_size = (topic_size < 5000) ? 5000 : topic_size;

    // Transport
    uxrUDPTransport transport;
    uxrUDPPlatform udp_platform;
    if(!uxr_init_udp_transport(&transport, &udp_platform, UXR_IPv4, ip, port))
    {
        printf("Error at create transport.\n");
        return 1;
    }

    // Session
    uxrSession session;
    uxr_init_session(&session, &transport.comm, 0xAAAABBBB);
    if(!uxr_create_session(&session))
    {
        printf("Error at create session.\n");
        return 1;
    }

    // Streams
    uint8_t * output_best_effort_buffer = (uint8_t*)malloc(buffer_size*sizeof(uint8_t));
    uxrStreamId best_effort_out = uxr_create_output_best_effort_stream(&session, output_best_effort_buffer, buffer_size);

    // Create entities
    uxrObjectId participant_id = uxr_object_id(0x01, UXR_PARTICIPANT_ID);
    const char* participant_xml = "<dds>"
                                      "<participant>"
                                          "<rtps>"
                                              "<name>default_xrce_participant</name>"
                                          "</rtps>"
                                      "</participant>"
                                  "</dds>";
    uint16_t participant_req = uxr_buffer_create_participant_xml(&session, best_effort_out, participant_id, 0, participant_xml, UXR_REPLACE);

    uxrObjectId topic_id = uxr_object_id(0x01, UXR_TOPIC_ID);
    const char* topic_xml = "<dds>"
                                "<topic>"
                                    "<name>SimpleArrayTopic</name>"
                                    "<dataType>SimpleArrayType</dataType>"
                                "</topic>"
                            "</dds>";
    uint16_t topic_req = uxr_buffer_create_topic_xml(&session, best_effort_out, topic_id, participant_id, topic_xml, UXR_REPLACE);

    uxrObjectId publisher_id = uxr_object_id(0x01, UXR_PUBLISHER_ID);
    const char* publisher_xml = "";
    uint16_t publisher_req = uxr_buffer_create_publisher_xml(&session, best_effort_out, publisher_id, participant_id, publisher_xml, UXR_REPLACE);

    uxrObjectId datawriter_id = uxr_object_id(0x01, UXR_DATAWRITER_ID);
    const char* datawriter_xml = "<dds>"
                                     "<data_writer>"
                                         "<qos>"
                                            "<reliability>"
                                            "<kind>BEST_EFFORT</kind>"
                                            "</reliability>"
                                        "</qos>"
                                         "<topic>"
                                             "<kind>NO_KEY</kind>"
                                             "<name>SimpleArrayTopic</name>"
                                             "<dataType>SimpleArrayType</dataType>"
                                         "</topic>"
                                     "</data_writer>"
                                 "</dds>";
    uint16_t datawriter_req = uxr_buffer_create_datawriter_xml(&session, best_effort_out, datawriter_id, publisher_id, datawriter_xml, UXR_REPLACE);

    uxr_flash_output_streams(&session);

    uint8_t * data = (uint8_t*)malloc(topic_size*sizeof(uint8_t));
    memset(data, 'a', topic_size);

    // Write topics
    while(1)
    {
        ucdrBuffer ub;
        uxr_prepare_output_stream(&session, best_effort_out, datawriter_id, &ub, topic_size);
        ucdr_serialize_array_char(&ub, data, topic_size);

        uxr_flash_output_streams(&session);
        usleep(1);
    }

    // Delete resources
    uxr_delete_session(&session);
    uxr_close_udp_transport(&transport);

    return 0;
}