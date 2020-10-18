#ifndef DATACONVERTER_H
#define DATACONVERTER_H

#include <stdint.h>
#include <stdlib.h>

#define NUM_TO_STR_BUF_SIZE     32

class DataConverter
{
    public:

        static float StringToFloat(char** text, uint32_t * success);
        static int StringToInteger(char** text, uint32_t * success);

        static float StringToFloat2(char* text, char** stopPos, uint32_t * success);
        static int StringToInteger2(char* text, char** stopPos, uint32_t * success);
    
        static const char* FloatToStringTruncate(float f, int maxlen = 0);
        static const char* IntegerToString(int32_t value);
    
    protected:
        static const uint32_t BIT_NEGATIVE = (1 << 15);
        static const uint32_t BIT_DIGITS   = (1 << 1);
        static const uint32_t BIT_DECIMAL   = (1 << 0);
    
        static char NumberToStrBuffer[NUM_TO_STR_BUF_SIZE];
};

#endif
