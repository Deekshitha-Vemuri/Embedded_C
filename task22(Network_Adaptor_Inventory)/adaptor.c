#include <stdio.h>    
#include <stdlib.h>    

#include <winsock2.h>   // SOCKADDR, SOCKADDR_IN, SOCKADDR_IN6
#include <windows.h>    // WideCharToMultiByte(), CP_UTF8

#include <iphlpapi.h>   // GetAdaptersAddresses(), adapter structures
#include <ws2tcpip.h>   // InetNtopA(), IP address string length constants


// Display the adapter's operational status in readable form
const char *getAdapterStatus(IF_OPER_STATUS status)
{
    switch (status)
    {
        case IfOperStatusUp:
            return "Up";

        case IfOperStatusDown:
            return "Down";

        case IfOperStatusTesting:
            return "Testing";

        case IfOperStatusUnknown:
            return "Unknown";

        case IfOperStatusDormant:
            return "Dormant";

        case IfOperStatusNotPresent:
            return "Not Present";

        case IfOperStatusLowerLayerDown:
            return "Lower Layer Down";

        default:
            return "Unknown";
    }
}

int main(void)
{
    // Initially, we don't know how much memory is needed
    ULONG bufferSize = 0;

    PIP_ADAPTER_ADDRESSES adapterAddresses = NULL;

    ULONG result;

    // Ask Windows how much memory is required for adapter information
    result = GetAdaptersAddresses(
        AF_UNSPEC,
        0,
        NULL,
        NULL,
        &bufferSize
    );

    // Check whether adapter information is available
    if (result == ERROR_NO_DATA)
    {
        printf("No network adapter information available.\n");
        return 0;
    }

    // Windows should tell us the required buffer size
    if (result != ERROR_BUFFER_OVERFLOW || bufferSize == 0)
    {
        printf("Failed to get the required buffer size. Error: %lu\n", result);
        return 1;
    }

    // Allocate memory based on the size provided by Windows
    adapterAddresses = malloc(bufferSize);

    if (adapterAddresses == NULL)
    {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // Try again with the allocated buffer
    // Retry if Windows says that more memory is required
    int success = 0;

    for (int attempt = 0; attempt < 3; attempt++)
    {
        result = GetAdaptersAddresses(
            AF_UNSPEC,
            0,
            NULL,
            adapterAddresses,
            &bufferSize
        );

        // Windows successfully filled our buffer
        if (result == NO_ERROR)
        {
            success = 1;
            break;
        }

        // The allocated memory is still too small
        if (result == ERROR_BUFFER_OVERFLOW)
        {
            PIP_ADAPTER_ADDRESSES temp =
                realloc(adapterAddresses, bufferSize);

            if (temp == NULL)
            {
                printf("Unable to resize the buffer.\n");
                free(adapterAddresses);
                return 1;
            }

            adapterAddresses = temp;
        }
        else
        {
            // Something else went wrong, so stop retrying
            break;
        }
    }

    // Check whether we managed to retrieve the adapter information
    if (!success)
    {
        printf("Failed to retrieve adapter information. Error: %lu\n", result);
        free(adapterAddresses);
        return 1;
    }

    printf("\n========== NETWORK ADAPTER INVENTORY ==========\n");

    // Start with the first adapter returned by Windows
    //here adaptor is a header pointer of returned available adaptors in form of linkedlist
    PIP_ADAPTER_ADDRESSES adapter = adapterAddresses;

    int adapterNumber = 1;

    // Go through every adapter one by one
    while (adapter != NULL)
    {
        char adapterName[512] = "Unknown";

        // FriendlyName is a wide-character string
        // Convert it to UTF-8 so that printf() can display it
        if (adapter->FriendlyName != NULL)
        {
            if (WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    adapter->FriendlyName,
                    -1,
                    adapterName,
                    sizeof(adapterName),
                    NULL,
                    NULL) == 0)
            {
                snprintf(adapterName, sizeof(adapterName), "Unknown");
            }
        }

        printf("\nAdapter %d:\n", adapterNumber);
        printf("Name   : %s\n", adapterName);

        // Display the current operational status
        printf("Status : %s\n", getAdapterStatus(adapter->OperStatus));

        int hasIPv4 = 0;
        int hasIPv6 = 0;

        // Get the first IP address assigned to this adapter
        // FirstUnicastAddress is the member of ADAPTER_UNICAST_ADDRESS struct.
        PIP_ADAPTER_UNICAST_ADDRESS address =
            adapter->FirstUnicastAddress;

        // An adapter can have multiple IP addresses, so check them all
        while (address != NULL)
        {
            // Make sure the socket address pointer is valid
            if (address->Address.lpSockaddr != NULL)
            {
                // Check whether the current address is IPv4
                if (address->Address.lpSockaddr->sa_family == AF_INET)
                {
                    //if the ipaddress is ipv4 it is of type sockaddr_in taken by windows by default
                    SOCKADDR_IN *ipv4 =
                        (SOCKADDR_IN *)address->Address.lpSockaddr;

                    char ipString[INET_ADDRSTRLEN];

                    // Convert binary IPv4 into readable text
                    if (InetNtopA(
                            AF_INET,
                            &ipv4->sin_addr,
                            ipString,
                            sizeof(ipString)) != NULL)
                    {
                        if (!hasIPv4)
                        {
                            printf("IPv4 Addresses:\n");
                        }

                        printf("    %s\n", ipString);
                        hasIPv4 = 1;
                    }
                }

                // Check whether the current address is IPv6
                else if (address->Address.lpSockaddr->sa_family == AF_INET6)
                {
                    SOCKADDR_IN6 *ipv6 =
                        (SOCKADDR_IN6 *)address->Address.lpSockaddr;

                    char ipString[INET6_ADDRSTRLEN];

                    // Convert binary IPv6 into readable text
                    if (InetNtopA(
                            AF_INET6,
                            &ipv6->sin6_addr,
                            ipString,
                            sizeof(ipString)) != NULL)
                    {
                        if (!hasIPv6)
                        {
                            printf("IPv6 Addresses:\n");
                        }

                        printf("    %s\n", ipString);
                        hasIPv6 = 1;
                    }
                }
            }

            // Move to the next IP address belonging to this adapter
            address = address->Next;
        }

        // Check whether this adapter has an IPv4 address
        if (!hasIPv4)
        {
            printf("IPv4 Addresses: Not Available\n");
        }

        // Check whether this adapter has an IPv6 address
        if (!hasIPv6)
        {
            printf("IPv6 Addresses: Not Available\n");
        }

        printf("-----------------------------------------------\n");

        // Move to the next network adapter
        adapter = adapter->Next;

        adapterNumber++;
    }

    printf("\n===============================================\n");

    // Release the allocated memory
    free(adapterAddresses);
    adapterAddresses = NULL;

    return 0;
}