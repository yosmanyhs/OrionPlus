#include "CoolantController.h"

#include <stm32f4xx_hal.h>
#include <ctype.h>
#include <string.h>

#include "settings_manager.h"
#include "pins.h"

CoolantController::CoolantController()
{    
}


CoolantController::~CoolantController()
{
}


void CoolantController::Stop()
{
    // Coolant Stop -> COOLANT_ENABLE_PIN = 0
    COOLANT_ENABLE_GPIO_Port->BSRR = (COOLANT_ENABLE_Pin << 16);
}

void CoolantController::EnableMist()
{
    // Coolant Mist -> COOLANT_ENABLE_PIN = 1
    COOLANT_ENABLE_GPIO_Port->BSRR = (COOLANT_ENABLE_Pin);
}

void CoolantController::EnableFlood()
{
    // Coolant Flood -> COOLANT_ENABLE_PIN = 1
    COOLANT_ENABLE_GPIO_Port->BSRR = (COOLANT_ENABLE_Pin);
}

