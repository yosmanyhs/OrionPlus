#include "SpindleController.h"

#include <ctype.h>
#include <string.h>

#include "settings_manager.h"

SpindleController::SpindleController()
{
    m_current_rpm = 0;
    m_current_tool_number = 0;
}


SpindleController::~SpindleController()
{
}

bool SpindleController::InmediateStop()
{
    return true;
}

