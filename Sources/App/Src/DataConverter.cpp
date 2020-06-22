#include "DataConverter.h"

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

