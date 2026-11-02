#include<stdio.h>
#include<windows.h>
int main(){
    // creating the SYSTEM_INFO struct to access the processor count;
    SYSTEM_INFO info;
    /*GetSystemInfo() is the method used to fill all the values like 
    noofprocessors,processortype,level etc by passing the system_info struct pointer.*/
    GetSystemInfo(&info);
    printf("ProcessorCount:%lu\n",(unsigned long)info.dwNumberOfProcessors);
    //memorystatusex is also a pre-defined struct to access the ram details
    MEMORYSTATUSEX mem;
    /*must initialize length because GlobalMemoryStatusEx()
     expects the caller to specify the size of the structure and if not api will fail.*/
    mem.dwLength = sizeof(mem);
    //GlobalMemoryStatusEx returns non zero value on success and 0 value on failure and The byte values are written into the MEMORYSTATUSEX struct.
    if(GlobalMemoryStatusEx(&mem)){
        printf("Total physical RAM: %llu bytes\n",
       (unsigned long long)mem.ullTotalPhys);
       //bytes are converted into GiB.
      double ramGB = (double)mem.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
      double avail_ramGB=(double) mem.ullAvailPhys/(1024.0*1024.0*1024.0);
//Total ram storage of the pc.      
printf("TOTAL RAM storage: %.2f GiB\n", ramGB);
//Available ram remaining 
printf("Available space:%.2f GiB\n",avail_ramGB);
    }
    else {
        printf("Failed to retrieve memory Information\n");
    }
    return 0;
}