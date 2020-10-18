#include "DataConverter.h"

char DataConverter::NumberToStrBuffer[NUM_TO_STR_BUF_SIZE];

//////////////////////////////////////////////////////////////////////////////////////////

float DataConverter::StringToFloat(char** text, uint32_t * success)
{
    float result = 0.0f;
	float exponent = 1.0f;
	int bits = 0;
	int ch;

	// Check for sign
	ch = *(*text);

	if (ch == '+')
		(*text)++;
	else if (ch == '-')
	{
		(*text)++;
		bits = DataConverter::BIT_NEGATIVE;
	}

	// Get integer part
	for ( ; (ch = *(*text)) != '\0'; (*text)++)
	{
		if (ch < '0' || ch > '9')
			break;

		result = (result * 10.0f) + (ch - '0');
		bits |= DataConverter::BIT_DIGITS;
	}

	// Check for decimal point
	if (ch == '.')
	{
		bits |= DataConverter::BIT_DECIMAL;
		(*text)++;
	}

	// Get decimal part
	for ( ; (ch = *(*text)) != '\0'; (*text)++)
	{
		if (ch < '0' || ch > '9')
			break;

		exponent *= 10.0f;
		result = (result * 10.0f) + (ch - '0');
		bits |= DataConverter::BIT_DIGITS;
	}

	// Prepare result
	result /= (exponent);

	if (bits & DataConverter::BIT_NEGATIVE)
		result *= -1.0f;

	// Update check variable
	if (success != NULL)
	{
		// success = 1 -> OK, success = 0 -> No digits
		*success = (((bits & DataConverter::BIT_DIGITS) != 0)) ? 1 : 0;
	}

	return (result);
}

int DataConverter::StringToInteger(char** text, uint32_t * success)
{
    int result = 0;
	int bits = 0;
	int ch;

	// Check for sign
	ch = *(*text);

	if (ch == '+')
		(*text)++;
	else if (ch == '-')
	{
		(*text)++;
		bits = DataConverter::BIT_NEGATIVE;
	}

	// Get integer part
	for ( ; (ch = *(*text)) != '\0'; (*text)++)
	{
		if (ch < '0' || ch > '9')
			break;

		result = (result * 10) + (ch - '0');
		bits |= BIT_DIGITS;
	}

	// Prepare result
	if (bits & DataConverter::BIT_NEGATIVE)
		result *= -1;

	// Update check variable
	if (success != NULL)
	{
		// success = 1 -> OK, success = 0 -> No digits
		*success = (((bits & DataConverter::BIT_DIGITS) != 0)) ? 1 : 0;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////

float DataConverter::StringToFloat2(char* text, char** stopPos, uint32_t * success)
{
    float result = 0.0f;
	float exponent = 1.0f;
	int bits = 0;
	int ch;

	// Check for sign
	ch = *text;

	if (ch == '+')
		text++;
	else if (ch == '-')
	{
		text++;
		bits = DataConverter::BIT_NEGATIVE;
	}

	// Get integer part
	for ( ; (ch = *text) != '\0'; text++)
	{
		if (ch < '0' || ch > '9')
			break;

		result = (result * 10.0f) + (ch - '0');
		bits |= DataConverter::BIT_DIGITS;
	}

	// Check for decimal point
	if (ch == '.')
	{
		bits |= DataConverter::BIT_DECIMAL;
		text++;
	}

	// Get decimal part
	for ( ; (ch = *text) != '\0'; text++)
	{
		if (ch < '0' || ch > '9')
			break;

		exponent *= 10.0f;
		result = (result * 10.0f) + (ch - '0');
		bits |= DataConverter::BIT_DIGITS;
	}

	// Prepare result
	result /= (exponent);

	if (bits & DataConverter::BIT_NEGATIVE)
		result *= -1.0f;

	// Update check variable
	if (success != NULL)
	{
		// success = 1 -> OK, success = 0 -> No digits
		*success = (((bits & DataConverter::BIT_DIGITS) != 0)) ? 1 : 0;
	}

    *stopPos = text;
	return (result);
}

int DataConverter::StringToInteger2(char* text, char** stopPos, uint32_t * success)
{
    int result = 0;
	int bits = 0;
	int ch;

	// Check for sign
	ch = *text;

	if (ch == '+')
		text++;
	else if (ch == '-')
	{
		text++;
		bits = DataConverter::BIT_NEGATIVE;
	}

	// Get integer part
	for ( ; (ch = *text) != '\0'; text++)
	{
		if (ch < '0' || ch > '9')
			break;

		result = (result * 10) + (ch - '0');
		bits |= BIT_DIGITS;
	}

	// Prepare result
	if (bits & DataConverter::BIT_NEGATIVE)
		result *= -1;

	// Update check variable
	if (success != NULL)
	{
		// success = 1 -> OK, success = 0 -> No digits
		*success = (((bits & DataConverter::BIT_DIGITS) != 0)) ? 1 : 0;
	}

    *stopPos = text;
	return result;
}

// This method is not thread safe. It returns a pointer to static data
// This data will be modified in a new call to FloatToStringTruncate or 
// IntegerToString. 

// The number format has 4 digits of precision
// The maxlen parameter allows to 'cut' the resulting text to the specified
// number of characters, including sign. In the case the last character after
// the text being truncated is a dot (.) then, it is also removed.
const char* DataConverter::FloatToStringTruncate(float f, int maxlen /* = 0 */)
{
    int i;
    unsigned long z, k;
    char *c = &DataConverter::NumberToStrBuffer[NUM_TO_STR_BUF_SIZE - 1];    // Point to the last position of buffer
    float f2,t;
    int   sign = 0;

    /* Unsigned long long only allows for 20 digits of precision
    * which is already more than double supports, so we limit the
    * digits to this.  long double might require an increase if it is ever
    * implemented.
    */
    if (f < 0.0f) 
    {
        sign = 1;
        f = -f;
    }

    k = f + 0.0001f;
    f2 = f - k;

    i = 1;
 
    while (f >= 10.0f) 
    {
        f /= 10.0f;
        i++;
    }

    // Terminate string
    *(c--)= '\0';

    t = f2 * 10000.0f;
    z = t + 0.5f;
    
    for (i = 0; i < 4; i++)
    {
        *(c--) = ('0' + (z % 10));
        z /= 10;
    }
    
    *(c--) = ('.');
    
    do 
    {
        *(c--) = ('0'+ (k % 10));
        k /= 10;
    }
    while (k);

    if (sign)
        *(c--) = ('-');
    
    if (maxlen > 0)
    {
        c[maxlen + 1] = '\0';

        // verificar que el ultimo caracter no sea el punto. si asi fuera eliminarlo
        if (c[maxlen] == '.')
            c[maxlen] = '\0';
    }

    return ++c;
}

// This method is not thread safe. It returns a pointer to static data
// This data will be modified in a new call to FloatToString or 
// IntegerToString. 
const char* DataConverter::IntegerToString(int32_t value)
{
    char *c;
    int sign = 0;

    c = &DataConverter::NumberToStrBuffer[NUM_TO_STR_BUF_SIZE - 1];

    if (value < 0) 
    {
        /* negative, so negate */
        value = -value;
    }
    
    // Terminate string
    *(c--)= '\0';
    
    do 
    {
        *(c--) = ('0'+ (value % 10));
        value /= 10;
    }
    while (value != 0);

    if (sign)
        *(c--) = ('-');
    
    return c;
}

