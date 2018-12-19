/*
Copyright (C) 2018. See AUTHORS.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "logging.h"
#include "http_sends.h"




void send_empty_response(const char *server_name, const int sockfd) {

  char time_str[32];
  char send_buffer[512];
  size_t send_len;
  time_t cur_time = time(NULL);

  strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&cur_time));

  send_len = snprintf(send_buffer, sizeof(send_buffer),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type:text/plain; charset=\"utf-8\"\r\n"
    "Connection:close\r\n"
    "Server: %s\r\n"
    "Date:%s\r\n"
    "Content-Length:0\r\n\r\n",
    server_name,
    time_str);


  send(sockfd, send_buffer, send_len, 0);

}



void send_error_response(const char *server_name, const int sockfd) {

  char time_str[32];
  char send_buffer[512];
  size_t send_len;
  time_t cur_time = time(NULL);

  strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&cur_time));

  send_len = snprintf(send_buffer, sizeof(send_buffer),
    "HTTP/1.1 500 Internal Server Error\r\n"
    "Content-Type:text/plain; charset=\"utf-8\"\r\n"
    "Connection:close\r\n"
    "Server: %s\r\n"
    "Date:%s\r\n"
    "Content-Length:0\r\n\r\n",
    server_name,
    time_str);


  send(sockfd, send_buffer, send_len, 0);

}





void send_root_xml_response(const char *uuid, const char *friendly_name,
                            const char *server_name, const int sockfd) {

  char time_str[32];
  char send_buffer[4096];
  char xml_buffer[2048];
  size_t send_len, content_len;
  time_t cur_time = time(NULL);

  strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&cur_time));
  

  content_len = snprintf(xml_buffer, sizeof(xml_buffer),
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
    "<specVersion><major>1</major><minor>0</minor></specVersion>"
    "<device><deviceType>urn:schemas-upnp-org:device:MediaServer:1"
    "</deviceType><friendlyName>%s"
    "</friendlyName><modelDescription>%s"
    "</modelDescription><modelName>Windows Media Connect compatible (%s"
    ")</modelName><modelNumber>1</modelNumber><serialNumber>12345678</serialNumber>"
    "<UDN>uuid:%s</UDN><dlna:X_DLNADOC xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">"
    "DMS-1.50</dlna:X_DLNADOC><presentationURL>/</presentationURL>"
    "<serviceList><service><serviceType>urn:schemas-upnp-org:service:ContentDirectory:1"
    "</serviceType><serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>"
    "<controlURL>/ctl/ContentDir</controlURL><eventSubURL>/evt/ContentDir</eventSubURL>"
    "<SCPDURL>/ContentDir.xml</SCPDURL></service>"
    "</serviceList></device></root>\r\n",
    friendly_name,
    friendly_name,
    friendly_name,
    uuid);


  send_len = snprintf(send_buffer, sizeof(send_buffer),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type:text/xml; charset=\"utf-8\"\r\n"
    "Connection:close\r\n"
    "Server:%s\r\n"
    "EXT: \r\n"
    "Date:%s\r\n"
    "Content-Length:%d\r\n\r\n"
    "%s",
    server_name,
    time_str,
    content_len,
    xml_buffer);


  send(sockfd, send_buffer, send_len, 0);

}





void send_content_dir_xml_response(const char *server_name, const int sockfd) {

  char time_str[32];
  char send_buffer[8192];
  size_t send_len;
  time_t cur_time = time(NULL);

  strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&cur_time));
  

  const char *xml_string = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
      "<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">"
      "<specVersion>"
      "<major>1</major>"
      "<minor>0</minor>"
      "</specVersion>"
      "<actionList>"
      "<action>"
      "<name>GetSearchCapabilities</name>"
      "<argumentList>"
      "<argument>"
      "<name>SearchCaps</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>SearchCapabilities</relatedStateVariable>"
      "</argument>"
      "</argumentList>"
      "</action>"
      "<action>"
      "<name>GetSortCapabilities</name>"
      "<argumentList>"
      "<argument>"
      "<name>SortCaps</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>SortCapabilities</relatedStateVariable>"
      "</argument>"
      "</argumentList>"
      "</action>"
      "<action>"
      "<name>GetSystemUpdateID</name>"
      "<argumentList>"
      "<argument>"
      "<name>Id</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>SystemUpdateID</relatedStateVariable>"
      "</argument>"
      "</argumentList>"
      "</action>"
      "<action>"
      "<name>Browse</name>"
      "<argumentList>"
      "<argument>"
      "<name>ObjectID</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_ObjectID</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>BrowseFlag</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_BrowseFlag</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>Filter</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Filter</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>StartingIndex</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Index</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>RequestedCount</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>SortCriteria</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_SortCriteria</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>Result</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Result</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>NumberReturned</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>TotalMatches</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>UpdateID</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>A_ARG_TYPE_UpdateID</relatedStateVariable>"
      "</argument>"
      "</argumentList>"
      "</action>"
      "<action>"
      "<name>Search</name>"
      "<argumentList>"
      "<argument>"
      "<name>ContainerID</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_ObjectID</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>SearchCriteria</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_SearchCriteria</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>Filter</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Filter</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>StartingIndex</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Index</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>RequestedCount</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>SortCriteria</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_SortCriteria</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>Result</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Result</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>NumberReturned</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>TotalMatches</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>UpdateID</name>"
      "<direction>out</direction>"
      "<relatedStateVariable>A_ARG_TYPE_UpdateID</relatedStateVariable>"
      "</argument>"
      "</argumentList>"
      "</action>"
      "<action>"
      "<name>UpdateObject</name>"
      "<argumentList>"
      "<argument>"
      "<name>ObjectID</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_ObjectID</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>CurrentTagValue</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_TagValueList</relatedStateVariable>"
      "</argument>"
      "<argument>"
      "<name>NewTagValue</name>"
      "<direction>in</direction>"
      "<relatedStateVariable>A_ARG_TYPE_TagValueList</relatedStateVariable>"
      "</argument>"
      "</argumentList>"
      "</action>"
      "</actionList>"
      "<serviceStateTable>"
      "<stateVariable sendEvents=\"yes\">"
      "<name>TransferIDs</name>"
      "<dataType>string</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_ObjectID</name>"
      "<dataType>string</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_Result</name>"
      "<dataType>string</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_SearchCriteria</name>"
      "<dataType>string</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_BrowseFlag</name>"
      "<dataType>string</dataType>"
      "<allowedValueList>"
      "<allowedValue>BrowseMetadata</allowedValue>"
      "<allowedValue>BrowseDirectChildren</allowedValue>"
      "</allowedValueList>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_Filter</name>"
      "<dataType>string</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_SortCriteria</name>"
      "<dataType>string</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_Index</name>"
      "<dataType>ui4</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_Count</name>"
      "<dataType>ui4</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_UpdateID</name>"
      "<dataType>ui4</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>A_ARG_TYPE_TagValueList</name>"
      "<dataType>string</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>SearchCapabilities</name>"
      "<dataType>string</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"no\">"
      "<name>SortCapabilities</name>"
      "<dataType>string</dataType>"
      "</stateVariable>"
      "<stateVariable sendEvents=\"yes\">"
      "<name>SystemUpdateID</name>"
      "<dataType>ui4</dataType>"
      "</stateVariable>"
      "</serviceStateTable>"
      "</scpd>\n";


  send_len = snprintf(send_buffer, sizeof(send_buffer),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type:text/xml; charset=\"utf-8\"\r\n"
    "Connection:close\r\n"
    "Server:%s\r\n"
    "Date:%s\r\n"
    "Content-Length:%d\r\n\r\n"
    "%s",
    server_name,
    time_str,
    strlen(xml_string),
    xml_string);


  send(sockfd, send_buffer, send_len, 0);

}





void send_content_response(const char *friendly_name, const char *server_name,
                           const char *server_url, const size_t sample_rate,
                           const size_t bit_depth, const int sockfd) {

  char time_str[32];
  char send_buffer[4096];
  char xml_buffer[2048];
  size_t send_len, content_len;
  time_t cur_time = time(NULL);

  strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&cur_time));


  content_len = snprintf(xml_buffer, sizeof(xml_buffer),
    "<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope "
    "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
    "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
    "<s:Body><u:BrowseResponse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\"><Result>"

    "&lt;DIDL-Lite xmlns:dc=&quot;http://purl.org/dc/elements/1.1/&quot; "
    "xmlns:upnp=&quot;urn:schemas-upnp-org:metadata-1-0/upnp/&quot; "
    "xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&quot; "
    "xmlns:pv=&quot;http://www.pv.com/pvns/&quot;&gt;&lt;item id=&quot;1&quot; parentID=&quot;0&quot; "
    "restricted=&quot;1&quot;&gt;&lt;dc:title&gt;%s"
    "&lt;/dc:title&gt;&lt;upnp:class&gt;object.item.audioItem.audioBroadcast&lt;/upnp:class&gt;"
    "&lt;upnp:channelNr&gt;1&lt;/upnp:channelNr&gt;&lt;upnp:channelName&gt;%s"
    "&lt;/upnp:channelName&gt;&lt;res protocolInfo=&quot;http-get:*:audio/x-flac:*&quot; "
    " sampleFrequency=&quot;%d&quot; bitsPerSample=&quot;%d&quot; &gt;"
    "%s/media/0.flac&lt;/res&gt;&lt;/item&gt;&lt;/DIDL-Lite&gt;"

    "</Result><NumberReturned>1</NumberReturned><TotalMatches>1</TotalMatches>"
    "<UpdateID>0</UpdateID></u:BrowseResponse></s:Body></s:Envelope>\r\n",
    friendly_name,
    friendly_name,
    sample_rate,
    bit_depth,
    server_url);


  send_len = snprintf(send_buffer, sizeof(send_buffer),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type:text/xml; charset=\"utf-8\"\r\n"
    "Connection:close\r\n"
    "Server:%s\r\n"
    "EXT: \r\n"
    "Date:%s\r\n"
    "Content-Length:%d\r\n\r\n"
    "%s",
    server_name,
    time_str,
    content_len,
    xml_buffer);


  send(sockfd, send_buffer, send_len, 0);


}






void send_chunked_stream_response(const char *server_name, const int sockfd) {

  char time_str[32];
  char send_buffer[2048];
  size_t send_len;
  time_t cur_time = time(NULL);

  strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&cur_time));

  send_len = snprintf(send_buffer, sizeof(send_buffer),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: audio/flac"
    "Connection: keep-alive\r\n"
    "Keep-Alive: timeout=30\r\n"
    "Transfer-Encoding: chunked\r\n"
    "Server: %s\r\n"
    "Date: %s\r\n\r\n",
    server_name,
    time_str);


  send(sockfd, send_buffer, send_len, 0);

}





FLAC__StreamEncoderWriteStatus send_flac_callback(const FLAC__StreamEncoder *encoder,
                                                  const FLAC__byte *buffer,
                                                  size_t bytes, unsigned samples,
                                                  unsigned current_frame,
                                                  void *client_data) {

  int sockfd = *((int*)client_data);


  


  size_t len_len;

  char len_str[32];

  len_len = snprintf(len_str, sizeof(len_str), "%x", bytes);

  send(sockfd, len_str, len_len, 0);
  send(sockfd, "\r\n", 2, 0);
  send(sockfd, buffer, bytes, 0);
  send(sockfd, "\r\n", 2, 0);

  return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}




