#include "FreeRTOS.h"
#include "task.h"

#pragma import(__use_no_semihosting)

void* operator new(size_t alloc_size)
{
    return pvPortMalloc(alloc_size);
}

void* operator new[](size_t alloc_size)
{
    return pvPortMalloc(alloc_size);
}

void operator delete(void* mem_to_free)
{
    vPortFree(mem_to_free);
}

void operator delete[](void* mem_to_free)
{
    vPortFree(mem_to_free);
}

////////////////////////////////////////////////////////////

//extern "C" int stderr_putchar (int ch)
//{
//    return 0;
//}

//extern "C" int stdin_getchar (void)
//{
//    return 0;
//}

//extern "C" int stdout_putchar (int ch)
//{
//    return 0;
//}

extern "C" void _sys_exit(int return_code)
{
    
}

extern "C" void _ttywrch(int ch)
{
    
}
