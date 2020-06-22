#ifndef DATACONVERTER_H
#define DATACONVERTER_H

#include <stdint.h>
#include <stdlib.h>

class DataConverter
{
    public:

        static float StringToFloat(char** text, uint32_t * success);
        static int StringToInteger(char** text, uint32_t * success);

        static float StringToFloat2(char* text, char** stopPos, uint32_t * success);
        static int StringToInteger2(char* text, char** stopPos, uint32_t * success);
    
    protected:
        static const uint32_t BIT_NEGATIVE = (1 << 15);
        static const uint32_t BIT_DIGITS   = (1 << 1);
        static const uint32_t BIT_DECIMAL   = (1 << 0);
};

#endif
