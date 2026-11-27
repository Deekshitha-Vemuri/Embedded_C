
/*
TASK 38 - WINDOWS EVENT LOGGER

BUILD COMMAND (Developer Command Prompt for VS):

set PATH=D:\Embedded_C\vcpkg\installed\x64-windows\bin;%PATH%
cl /nologo /W4 /std:c17 /TC main.c /I"D:\Embedded_C\vcpkg\installed\x64-windows\include\libxml2" /Fe:main.exe /link /LIBPATH:"D:\Embedded_C\vcpkg\installed\x64-windows\lib" wevtapi.lib libxml2.lib iconv.lib charset.lib libz.lib
*/

#include <windows.h>
#include <winevt.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#define MAX_EVENTS 15

// Find a child element using its name.
static xmlNodePtr findChildElement(xmlNodePtr parent, const char *name)
{
    xmlNodePtr current = NULL;

    for (current = parent->children;
         current != NULL;
         current = current->next)
    {
        if (current->type == XML_ELEMENT_NODE &&
            xmlStrEqual(current->name, BAD_CAST name))
        {
            return current;
        }
    }

    return NULL;
}

// Extract the numeric content of an XML element.
static int readElementValue(
    xmlNodePtr parent,
    const char *elementName,
    long *result)
{
    xmlNodePtr element = NULL;
    xmlChar *content = NULL;

    char *endPointer = NULL;
    long value = 0;

    element = findChildElement(parent, elementName);

    if (element == NULL)
    {
        printf("Element %s not found.\n", elementName);
        return 0;
    }

    // Read the text inside the element.
    content = xmlNodeGetContent(element);

    if (content == NULL)
    {
        printf("Failed to read %s content.\n", elementName);
        return 0;
    }

    // Convert the extracted string into a decimal integer.
    errno = 0;

    value = strtol((const char *)content, &endPointer, 10);

    if (endPointer == (char *)content || errno == ERANGE)
    {
        printf("Invalid numeric value in %s.\n", elementName);
        xmlFree(content);
        return 0;
    }

    // Check that no unexpected characters remain.
    while (isspace((unsigned char)*endPointer))
    {
        endPointer++;
    }

    if (*endPointer != '\0')
    {
        printf("Unexpected characters in %s.\n", elementName);
        xmlFree(content);
        return 0;
    }

    *result = value;

    // Release the memory allocated by xmlNodeGetContent().
    xmlFree(content);

    return 1;
}

// Render an event handle into an XML string.
static WCHAR *renderEventXml(EVT_HANDLE eventHandle)
{
    DWORD bufferUsed = 0;
    DWORD propertyCount = 0;

    DWORD renderedBytes = 0;

    WCHAR *xmlWideBuffer = NULL;

    // First call: find the required buffer size.
    if (EvtRender(
            NULL,
            eventHandle,
            EvtRenderEventXml,
            0,
            NULL,
            &bufferUsed,
            &propertyCount))
    {
        printf("Unexpected result while checking XML buffer size.\n");
        return NULL;
    }

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        printf("Failed to calculate XML buffer size. Error: %lu\n",
               (unsigned long)GetLastError());
        return NULL;
    }

    if (bufferUsed == 0)
    {
        printf("Invalid XML buffer size.\n");
        return NULL;
    }

    // Allocate memory for the XML output.
    xmlWideBuffer = (WCHAR *)malloc(bufferUsed);

    if (xmlWideBuffer == NULL)
    {
        printf("Memory allocation failed for XML buffer.\n");
        return NULL;
    }

    // Second call: render the event into the allocated buffer.
    if (!EvtRender(
            NULL,
            eventHandle,
            EvtRenderEventXml,
            bufferUsed,
            xmlWideBuffer,
            &renderedBytes,
            &propertyCount))
    {
        printf("Failed to render event XML. Error: %lu\n",
               (unsigned long)GetLastError());

        free(xmlWideBuffer);
        return NULL;
    }

    return xmlWideBuffer;
}

// Convert UTF-16 XML into UTF-8 XML.
static char *convertXmlToUtf8(const WCHAR *xmlWideBuffer)
{
    int requiredSize = 0;
    int convertedSize = 0;

    char *xmlUtf8 = NULL;

    // First call: calculate the required UTF-8 buffer size.
    requiredSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        xmlWideBuffer,
        -1,
        NULL,
        0,
        NULL,
        NULL);

    if (requiredSize == 0)
    {
        printf("Failed to calculate UTF-8 buffer size. Error: %lu\n",
               (unsigned long)GetLastError());
        return NULL;
    }

    // Allocate memory for the UTF-8 output.
    xmlUtf8 = (char *)malloc((size_t)requiredSize);

    if (xmlUtf8 == NULL)
    {
        printf("Memory allocation failed for UTF-8 buffer.\n");
        return NULL;
    }

    // Second call: perform the UTF-16 to UTF-8 conversion.
    convertedSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        xmlWideBuffer,
        -1,
        xmlUtf8,
        requiredSize,
        NULL,
        NULL);

    if (convertedSize == 0)
    {
        printf("UTF-8 conversion failed. Error: %lu\n",
               (unsigned long)GetLastError());

        free(xmlUtf8);
        return NULL;
    }

    return xmlUtf8;
}

// Parse the XML and extract Event ID, Level and Timestamp.
static int processEvent(EVT_HANDLE eventHandle, DWORD eventNumber)
{
    WCHAR *xmlWideBuffer = NULL;
    char *xmlUtf8 = NULL;

    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL;
    xmlNodePtr systemNode = NULL;

    xmlNodePtr timeCreatedNode = NULL;
    xmlChar *timestamp = NULL;

    long eventId = 0;
    long level = 0;

    int success = 0;
    size_t xmlLength = 0;

    // Step 1: Render the event into UTF-16 XML.
    xmlWideBuffer = renderEventXml(eventHandle);

    if (xmlWideBuffer == NULL)
    {
        goto cleanup;
    }

    // Step 2: Convert the XML into UTF-8.
    xmlUtf8 = convertXmlToUtf8(xmlWideBuffer);

    if (xmlUtf8 == NULL)
    {
        goto cleanup;
    }

    // Step 3: Parse the UTF-8 XML using libxml2.
    xmlLength = strlen(xmlUtf8);

    if (xmlLength > INT_MAX)
    {
        printf("XML document is too large to parse.\n");
        goto cleanup;
    }

    doc = xmlReadMemory(
        xmlUtf8,
        (int)xmlLength,
        NULL,
        NULL,
        XML_PARSE_NONET);

    if (doc == NULL)
    {
        printf("XML parsing failed for event %lu.\n",
               (unsigned long)eventNumber);
        goto cleanup;
    }

    // Step 4: Get the root element.
    root = xmlDocGetRootElement(doc);

    if (root == NULL)
    {
        printf("XML document has no root element.\n");
        goto cleanup;
    }

    // Step 5: Find the System element.
    systemNode = findChildElement(root, "System");

    if (systemNode == NULL)
    {
        printf("System element not found.\n");
        goto cleanup;
    }

    // Step 6: Extract EventID and Level.
    if (!readElementValue(systemNode, "EventID", &eventId))
    {
        goto cleanup;
    }

    if (!readElementValue(systemNode, "Level", &level))
    {
        goto cleanup;
    }

    // Step 7: Find the TimeCreated element.
    timeCreatedNode = findChildElement(systemNode, "TimeCreated");

    if (timeCreatedNode == NULL)
    {
        printf("TimeCreated element not found.\n");
        goto cleanup;
    }

    // Step 8: Extract the SystemTime attribute.
    timestamp = xmlGetProp(
        timeCreatedNode,
        BAD_CAST "SystemTime"
    );

    if (timestamp == NULL)
    {
        printf("SystemTime attribute not found.\n");
        goto cleanup;
    }

    // Step 9: Display the extracted information.
    printf("Event %lu\n", (unsigned long)eventNumber);
    printf("Timestamp: %s\n", (const char *)timestamp);
    printf("Event ID : %ld\n", eventId);
    printf("Level    : %ld\n\n", level);

    success = 1;

cleanup:

    // Release the timestamp allocated by xmlGetProp().
    if (timestamp != NULL)
    {
        xmlFree(timestamp);
    }

    // Release the parsed XML document.
    if (doc != NULL)
    {
        xmlFreeDoc(doc);
    }

    // Release the UTF-8 buffer.
    free(xmlUtf8);

    // Release the UTF-16 buffer.
    free(xmlWideBuffer);

    return success;
}

int main(void)
{
    EVT_HANDLE queryHandle = NULL;
    EVT_HANDLE eventHandles[MAX_EVENTS] = {0};

    DWORD returned = 0;
    DWORD errorCode = ERROR_SUCCESS;

    DWORD processed = 0;

    // Step 1: Query the Application event log.
    queryHandle = EvtQuery(
        NULL,
        L"Application",
        NULL,
        EvtQueryChannelPath | EvtQueryReverseDirection);

    if (queryHandle == NULL)
    {
        printf("Failed to query Application log. Error: %lu\n",
               (unsigned long)GetLastError());
        return 1;
    }

    printf("Application Event Logger\n");
    printf("========================\n\n");

    // Step 2: Retrieve up to 15 newest events.
    BOOL nextSuccess = EvtNext(
        queryHandle,
        MAX_EVENTS,
        eventHandles,
        0,
        0,
        &returned);

    if (!nextSuccess)
    {
        errorCode = GetLastError();

        // ERROR_NO_MORE_ITEMS can also occur when fewer than 15
        // events are available. Process any events already returned.
        if (errorCode != ERROR_NO_MORE_ITEMS)
        {
            printf("EvtNext failed. Error: %lu\n",
                   (unsigned long)errorCode);
        }
    }

    if (returned == 0)
    {
        printf("No events found in Application log.\n");
    }

    // Step 3: Process each retrieved event.
    for (DWORD i = 0; i < returned; i++)
    {
        if (processEvent(eventHandles[i], i + 1))
        {
            processed++;
        }

        // Close the current event handle.
        EvtClose(eventHandles[i]);
        eventHandles[i] = NULL;
    }

    printf("========================\n");
    printf("Events retrieved : %lu\n", (unsigned long)returned);
    printf("Events processed : %lu\n", (unsigned long)processed);

    // Step 4: Close the query handle.
    EvtClose(queryHandle);

    return 0;
}
