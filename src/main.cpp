#include <stdio.h>


#include "include/defines.h"
#include "include/vk_engine.h"

void main()
{
    engine.init();
    printf("Hello world\n");
    engine.cleanup();
}
