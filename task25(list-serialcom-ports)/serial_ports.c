
/*
    Task 25: List available serial COM ports

    Build using:
    cl /nologo /W4 /TC /std:c11 serial_ports.c /link setupapi.lib advapi32.lib

    Headers used:
    windows.h  - Windows data types, error codes, memory functions and registry types.
    setupapi.h - SetupDiGetClassDevs, SetupDiEnumDeviceInterfaces,
                 SetupDiGetDeviceInterfaceDetail, SetupDiGetDeviceRegistryProperty,
                 SetupDiOpenDevRegKey and SetupDiDestroyDeviceInfoList.
    ntddser.h  - GUID_DEVINTERFACE_COMPORT, which identifies COM port interfaces.
    stdio.h    - printf and wprintf for displaying results.
    stdlib.h   - malloc and free for allocating and releasing memory.
    wchar.h    - Wide-character string support.
    initguid.h - Defines the COM port GUID in this source file.
*/

#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

/*
    Read a device property, such as its friendly name.

    We first ask Windows how much memory is required.
    Then we allocate that memory and retrieve the actual string.
*/
WCHAR* GetDevicePropertyString(
    HDEVINFO deviceInfoSet,
    SP_DEVINFO_DATA* deviceInfoData,
    DWORD property
)
{
    DWORD requiredSize = 0;
    DWORD propertyType = 0;

    // First call: Ask Windows for the required buffer size.
    SetupDiGetDeviceRegistryPropertyW(
        deviceInfoSet,       // Collection containing our device.
        deviceInfoData,      // Identifies the device whose property we need.
        property,            // Specifies which property to retrieve.
        &propertyType,       // Receives the registry data type.
        NULL,                // We have not allocated the buffer yet.
        0,                   // No buffer is provided.
        &requiredSize        // Receives the required size in bytes.
    );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
        requiredSize == 0)
    {
        return NULL;
    }

    // Allocate enough memory for the device name and its null terminator.
    WCHAR* propertyBuffer =
        (WCHAR*)calloc(1, requiredSize + sizeof(WCHAR));

    if (propertyBuffer == NULL)
    {
        return NULL;
    }

    // Second call: Retrieve the actual device property.
    if (!SetupDiGetDeviceRegistryPropertyW(
        deviceInfoSet,
        deviceInfoData,
        property,
        &propertyType,
        (PBYTE)propertyBuffer,   // Windows writes the property here.
        requiredSize,            // Size of the allocated buffer.
        &requiredSize
    ))
    {
        free(propertyBuffer);
        return NULL;
    }

    if (propertyType != REG_SZ)
    {
        free(propertyBuffer);
        return NULL;
    }

    return propertyBuffer;
}

/*
    Read the COM port name from the device's registry key.

    Example: PortName = COM5
*/
WCHAR* GetComPortName(
    HDEVINFO deviceInfoSet,
    SP_DEVINFO_DATA* deviceInfoData
)
{
    // Open the device-specific registry key to read its COM port name.
    HKEY deviceKey = SetupDiOpenDevRegKey(
        deviceInfoSet,          // Collection containing our device.
        deviceInfoData,         // Identifies the particular device.
        DICS_FLAG_GLOBAL,       // Read the global device configuration.
        0,                      // No specific hardware profile.
        DIREG_DEV,              // Open the device-specific registry key.
        KEY_QUERY_VALUE         // We only need permission to read values.
    );

    if (deviceKey == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    DWORD valueType = 0;
    DWORD requiredSize = 0;

    // First call: Ask the registry how much memory PortName needs.
    LSTATUS status = RegQueryValueExW(
        deviceKey,
        L"PortName",             // This registry value contains COM5, COM6, etc.
        NULL,                    // Reserved parameter.
        &valueType,              // Receives the registry data type.
        NULL,                    // We are only checking the required size.
        &requiredSize            // Receives the required size in bytes.
    );

    if (status != ERROR_SUCCESS ||
        valueType != REG_SZ ||
        requiredSize == 0)
    {
        RegCloseKey(deviceKey);
        return NULL;
    }

    // Allocate enough memory for the COM name.
    WCHAR* portName =
        (WCHAR*)calloc(1, requiredSize + sizeof(WCHAR));

    if (portName == NULL)
    {
        RegCloseKey(deviceKey);
        return NULL;
    }

    // Second call: Read the actual PortName value from the registry.
    DWORD actualSize = requiredSize;

    status = RegQueryValueExW(
        deviceKey,
        L"PortName",             // Read the COM port name.
        NULL,
        &valueType,
        (LPBYTE)portName,        // Receives the value, such as COM5.
        &actualSize              // Receives the actual data size.
    );

    // We have finished reading this device's registry information.
    RegCloseKey(deviceKey);

    if (status != ERROR_SUCCESS || valueType != REG_SZ)
    {
        free(portName);
        return NULL;
    }

    return portName;
}

int main(void)
{
    DWORD index = 0;
    DWORD foundCount = 0;

    /*
        STEP 1: Get the collection of present COM port interfaces.

        GUID_DEVINTERFACE_COMPORT identifies serial port interfaces.
        DIGCF_PRESENT includes only devices currently present.
        DIGCF_DEVICEINTERFACE tells Windows to retrieve device interfaces.
    */

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
        &GUID_DEVINTERFACE_COMPORT,       // We want COM port interfaces.
        NULL,                             // Search across all enumerators.
        NULL,                             // No parent window is required.
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Failed to retrieve serial devices. Error: %lu\n",
                GetLastError());
        return 1;
    }

    wprintf(L"\nAvailable serial COM ports:\n\n");

    /*
        STEP 2: Enumerate the COM port interfaces one by one.

        The index starts at zero.
        Each successful call gives us one interface.
    */

    while (1)
    {
        SP_DEVICE_INTERFACE_DATA interfaceData = { 0 };

        // Windows needs the size of this structure before filling it.
        interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        BOOL result = SetupDiEnumDeviceInterfaces(
            deviceInfoSet,                    // Collection obtained earlier.
            NULL,                             // Do not restrict to one device.
            &GUID_DEVINTERFACE_COMPORT,       // We want COM port interfaces.
            index,                            // Select the current interface.
            &interfaceData                    // Receives interface information.
        );

        if (!result)
        {
            DWORD error = GetLastError();

            if (error == ERROR_NO_MORE_ITEMS)
            {
                // No more serial interfaces are available.
                break;
            }

            wprintf(L"Enumeration failed. Error: %lu\n", error);
            break;
        }

        /*
            STEP 3: Get the detailed information about this interface.

            The first call tells us how much memory the device path needs.
        */

        DWORD requiredSize = 0;

        SetupDiGetDeviceInterfaceDetailW(
            deviceInfoSet,
            &interfaceData,
            NULL,                    // First call: no detail buffer.
            0,                       // No buffer size yet.
            &requiredSize,           // Receives the required size.
            NULL
        );

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
            requiredSize == 0)
        {
            wprintf(L"Could not get interface detail buffer size. Error: %lu\n",
                    GetLastError());

            index++;
            continue;
        }

        // Allocate enough memory for the complete interface details.
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detailData =
            (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(requiredSize);

        if (detailData == NULL)
        {
            wprintf(L"Memory allocation failed.\n");
            break;
        }

        // Windows needs the structure size before filling the device path.
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        SP_DEVINFO_DATA deviceInfoData = { 0 };

        // Initialize this structure so Windows can fill device information.
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        /*
            Second call: Retrieve the actual interface details.

            detailData receives the device path.
            deviceInfoData identifies the device behind this interface.
        */

        if (!SetupDiGetDeviceInterfaceDetailW(
            deviceInfoSet,
            &interfaceData,           // Interface obtained during enumeration.
            detailData,               // Receives the device interface path.
            requiredSize,             // Size of our allocated buffer.
            NULL,                     // We already know the required size.
            &deviceInfoData           // Receives the associated device information.
        ))
        {
            wprintf(L"Could not retrieve interface details. Error: %lu\n",
                    GetLastError());

            free(detailData);
            index++;
            continue;
        }

        /*
            STEP 4: Retrieve the friendly device name.

            deviceInfoData identifies the device.
            SPDRP_FRIENDLYNAME asks Windows for its friendly name.
        */

        WCHAR* friendlyName = GetDevicePropertyString(
            deviceInfoSet,
            &deviceInfoData,
            SPDRP_FRIENDLYNAME
        );

        // If no friendly name is available, try the device description.
        if (friendlyName == NULL)
        {
            friendlyName = GetDevicePropertyString(
                deviceInfoSet,
                &deviceInfoData,
                SPDRP_DEVICEDESC
            );
        }

        /*
            STEP 5: Retrieve the COM port name from the Windows Registry.

            We open the device-specific registry key.
            Then we read its PortName value using RegQueryValueExW.
        */

        WCHAR* portName = GetComPortName(
            deviceInfoSet,
            &deviceInfoData
        );

        /*
            STEP 6: Display the information.

            We need both values to display the COM port and device name.
        */

        if (portName != NULL)
        {
            wprintf(L"%ls : %ls\n",
                    portName,
                    friendlyName != NULL ? friendlyName : L"Unknown device");

            foundCount++;
        }

        // Release the memory allocated for this device.
        free(portName);
        free(friendlyName);
        free(detailData);

        // Move to the next interface.
        index++;
    }

    /*
        STEP 7: Display the result if no serial devices were found.
    */

    if (foundCount == 0)
    {
        wprintf(L"No serial COM ports detected.\n");
    }
    else
    {
        wprintf(L"\nTotal serial COM ports found: %lu\n", foundCount);
    }

    /*
        STEP 8: Release the device information collection.

        This frees the resources created by SetupDiGetClassDevsW.
    */

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return 0;
}
