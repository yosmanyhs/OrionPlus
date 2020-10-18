#include "GCodeParser.h"


#include <ctype.h>
#include <string.h>
#include <math.h>

#include "settings_manager.h"

#include "user_tasks.h"
#include "MachineCore.h"

GCodeParser::GCodeParser(void)
{
    ResetParser();
    
    m_planner_ref = NULL;
}


GCodeParser::~GCodeParser(void)
{
}

void GCodeParser::ResetParser()
{
    uint32_t loop_index;
    
    m_line = NULL;
    
    m_spindle_speed = 0.0f;
    m_feed_rate = 0.0f;
    m_tool_number = 0;

	m_axis_zero   = COORD_X;
	m_axis_one    = COORD_Y;
	m_axis_linear = COORD_Z;

    m_value_group_flags = 0;
    m_axis_command_type = AXIS_COMMAND_TYPE_NONE;

    reset_modal_params();
    
    memset((void*)m_work_coord_sys, 0, sizeof(m_work_coord_sys));
    memset((void*)m_g92_coord_offset, 0, sizeof(m_g92_coord_offset));
    memset((void*)m_tool_offset, 0, sizeof(m_tool_offset));
    memset((void*)m_last_probe_position, 0, sizeof(m_last_probe_position));
    memset((void*)m_gcode_machine_pos, 0, sizeof(m_gcode_machine_pos));

    memset((void*)&m_block_data, 0, sizeof(m_block_data));
    
    // Load some information from settings
    for (loop_index = COORD_X; loop_index <= COORD_Z; loop_index++)
    {
        m_soft_limit_enabled[loop_index] = Settings_Manager::AreAxisSoftLimitEnabled(loop_index);
        m_soft_limit_max_values[loop_index] = Settings_Manager::GetMaxTravel_mm_Axis(loop_index);
    }
    
    m_check_mode = false;
    
    m_mm_max_arc_error = Settings_Manager::GetMaxArcError_mm();
    m_mm_per_arc_segment = Settings_Manager::GetArcSegmentSize_mm();
    m_arc_correction_counter = 5;
    
    // Load G92 offsets from the settings
    Settings_Manager::ReadCoordinateValues(92, &m_g92_coord_offset[0]);
    
    m_canned_cycle_active = false;
        
    m_cc_initial_z = 0.0f;
    m_cc_r_plane = 0.0f;
    
    canned_cycle_reset_stycky();
}

int GCodeParser::ParseLine(char * line)
{
    uint32_t line_len = 0;
	uint32_t modal_group_flags = 0;
    int32_t work_var;
    char ch;
    
    m_line = line;
	m_value_group_flags = 0;
    m_axis_command_type = AXIS_COMMAND_TYPE_NONE;
    
    // Start parsing
    // Check/Clean up the input line [remove comments and whitespace too]
    work_var = cleanup_and_lowercase_line(line_len);

    if (work_var != GCODE_OK || line_len == 0)
        return work_var;
    
    // Reset all block values
	memset((void*)&m_block_data, 0, sizeof(m_block_data));
    
    // Copy parser's current modal states to this block
    memcpy((void*)&m_block_data.block_modal_state, (const void*)&m_parser_modal_state, sizeof(m_parser_modal_state));

    // Copy other parameters to current block (modified if words found)
    m_block_data.feed_rate = m_feed_rate;           // Keep current feed rate unless it is modified after
    m_block_data.spindle_speed = m_spindle_speed;   // Keep current spindle speed
    m_block_data.tool_number = m_tool_number;

    // Iterate over all line until end of string or error is found
    while ((ch = *m_line++) != '\0')
    {
        float value;
        uint32_t success_bits;
        
        switch (ch)
		{
			case '/':   // Block delete, skip this line completely
				return GCODE_INFO_BLOCK_DELETE;

			case 'n':
			{
				// Read line number
                work_var = DataConverter::StringToInteger(&m_line, &success_bits);

				// Check for numbers >= 0 
				if (work_var < 0 || success_bits == 0)
                    return GCODE_ERROR_INVALID_LINE_NUMBER;
			}
			break;

            case 'd':
            {
                // Check for multiple definitions of D word
                if ((m_value_group_flags & VALUE_SET_D_BIT) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_D_WORD; // Multiple definitions of D word value in the same line
                
                // Read integer value
                work_var = DataConverter::StringToInteger(&m_line, &success_bits);

                if (success_bits == 0 || work_var < 0)
                    return GCODE_ERROR_INVALID_D_VALUE; // 
                
                // Finally update D value and bitmap
                m_block_data.D_value = work_var;
                m_value_group_flags |= VALUE_SET_D_BIT;
            }
            break;
            
            case 'h':
            {
                // Check for multiple definitions of H word
                if ((m_value_group_flags & VALUE_SET_H_BIT) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_H_WORD; // Multiple definitions of D word value in the same line
                
                // Read integer value
                work_var = DataConverter::StringToInteger(&m_line, &success_bits);

                if (success_bits == 0 || work_var < 0)
                    return GCODE_ERROR_INVALID_H_VALUE; 
                
                // Finally update H value and bitmap
                m_block_data.H_value = work_var;
                m_value_group_flags |= VALUE_SET_H_BIT;
            }
            break;

            case 'q':
            {
                // Check for multiple definitions of Q word
                if ((m_value_group_flags & VALUE_SET_Q_BIT) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_Q_WORD; // Multiple definitions of Q word value in the same line

                // read q value
                value = DataConverter::StringToFloat(&m_line, &success_bits);

                if (success_bits == 0 || value <= 0.0f)
                    return GCODE_ERROR_INVALID_Q_VALUE;                

				// Finally update Q value and bitmap
				m_block_data.Q_value = value;
                m_value_group_flags |= VALUE_SET_Q_BIT;
            }
            break;

			case 'a':
            case 'b':
            case 'c':
            {
                work_var = ((int)((ch - 'a') + COORDINATE_LINEAR_AXES_COUNT));

                // Check for multiple inclusions in this line
                // A @ 3, B @ 4, C @ 5
                if (((1 << work_var) & m_value_group_flags) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_SAME_COORD; // Multiple definition of a,b,c coordinates

				// Try to read float value of coordinate
                value = DataConverter::StringToFloat(&m_line, &success_bits);

				if (success_bits == 0)
                    return GCODE_ERROR_INVALID_COORDINATE_VALUE;

                // Update data and bitmap with current coordinate usage bit index
                m_block_data.coordinate_data[work_var] = value;
				m_value_group_flags |= (1 << work_var);
            }
            break;
            
            case 'x':
			case 'y':
			case 'z':
			{
                work_var = ((int)(ch - 'x')); // map x -> 0, y -> 1, z -> 2

				// Check for multiple inclusions in this line

                // X @ 0, Y @ 1, Z @ 2
                if (((1 << work_var) & m_value_group_flags) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_SAME_COORD; // Multiple definition of x,y,z, a coordinates

				// Try to read float value of coordinate
                value = DataConverter::StringToFloat(&m_line, &success_bits);

				if (success_bits == 0)
                    return GCODE_ERROR_INVALID_COORDINATE_VALUE;

                // Update data and bitmap with current coordinate usage bit index
				m_block_data.coordinate_data[work_var] = value;
				m_value_group_flags |= (1 << work_var);
			}
			break;
            
            case 'i':
			case 'j':
			case 'k':
			{
				work_var = ((int)(ch - 'i')) + (TOTAL_AXES_COUNT); // maps i -> 6, j -> 7, k -> 8 (see bitmap in GCodeParser.h)

				// Check for multiple inclusions in this line
				if (((1 << work_var) & m_value_group_flags) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_SAME_OFFSET;    // Multiple definition of i,j,k offset

				// Try to read float value of coordinate
                value = DataConverter::StringToFloat(&m_line, &success_bits);

				if (success_bits == 0)
                    return  GCODE_ERROR_INVALID_OFFSET_VALUE;

                // Update data and bitmap bits
                m_block_data.offset_ijk_data[work_var - (TOTAL_AXES_COUNT)] = value;
				m_value_group_flags |= (1 << work_var);
			}
			break;
            
            case 'f':
			{
				// Check for multiple definitions of F word
                if ((m_value_group_flags & VALUE_SET_F_BIT) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_FEEDRATE;   // Multiple definition of feed rate in the same line

				// Try to read feed rate float value
                value = DataConverter::StringToFloat(&m_line, &success_bits);
                
				if (success_bits == 0 || value < 0.0f)
                    return GCODE_ERROR_INVALID_FEEDRATE_VALUE;  // Invalid feed rate value

				// Finally update feedrate and bitmap
                m_block_data.feed_rate = value;
                m_value_group_flags |= VALUE_SET_F_BIT;
			}
			break;
            
            case 'l':
			{
				if ((m_value_group_flags & VALUE_SET_L_BIT) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_L_WORD;

				// Read integer
                work_var = DataConverter::StringToInteger(&m_line, &success_bits);

				if (success_bits == 0 || work_var < 0)
                    return GCODE_ERROR_INVALID_L_VALUE;

                // Update L value and bitmap
				m_block_data.L_value = work_var;
                m_value_group_flags |= VALUE_SET_L_BIT;
			}
			break;
            
            case 'p':
			{
				// Dwell time parameter / G10 selector
				if ((m_value_group_flags & VALUE_SET_P_BIT) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_P_WORD;

				// Read float
                value = DataConverter::StringToFloat(&m_line, &success_bits);

				if (success_bits == 0 || value < 0.0f)
                    return GCODE_ERROR_INVALID_P_VALUE;

                // Update data and bitmap
				m_block_data.P_value = value;
                m_value_group_flags |= VALUE_SET_P_BIT;
			}
			break;
            
            case 'r':
			{
				// Arc Radius / R value for canned cycles
				if ((m_value_group_flags & VALUE_SET_R_BIT) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_R_WORD;

                value = DataConverter::StringToFloat(&m_line, &success_bits);

				if (success_bits == 0)
                    return GCODE_ERROR_INVALID_R_VALUE;

                // Update data and bitmap
                m_block_data.R_value = value;
                m_value_group_flags |= VALUE_SET_R_BIT;
			}
			break;

            case 's':
			{
				// Spindle speed RPMs
				if ((m_value_group_flags & VALUE_SET_S_BIT) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_SPINDLE_SPEED;

				value = DataConverter::StringToFloat(&m_line, &success_bits);

				if (success_bits == 0 || value < 0.0f)
                    return GCODE_ERROR_INVALID_SPINDLE_SPEED;

                // Update data and bitmap
				m_block_data.spindle_speed = value;
                m_value_group_flags |= VALUE_SET_S_BIT;
			}
			break;
            
            case 't':
			{
				// Tool index number
				if ((m_value_group_flags & VALUE_SET_T_BIT) != 0)
                    return GCODE_ERROR_MULTIPLE_DEF_TOOL_INDEX;

                work_var = DataConverter::StringToInteger(&m_line, &success_bits);

				if (success_bits == 0 || work_var < 0)
                    return GCODE_ERROR_INVALID_TOOL_INDEX;

                // update data and bitmap
				m_block_data.tool_number = work_var;
                m_value_group_flags |= VALUE_SET_T_BIT;
			}
			break;
            
            case 'm':
			{
                // Try to read integer value
                work_var = DataConverter::StringToInteger(&m_line, &success_bits);
                
                if (work_var < 0 || work_var > 99 || success_bits == 0)
                    return GCODE_ERROR_INVALID_M_CODE;

                // Parse specific codes
                switch (work_var)
                {
                    case MODAL_FLOW_TEMPORARY_STOP:             // M0   (Feed Hold)
                    case MODAL_FLOW_OPTIONAL_TEMP_STOP:         // M1   (ignore)
                    case MODAL_FLOW_COMPLETED:                  // M2
                    case MODAL_FLOW_COMPLETED_PALLET:           // M30
                        m_block_data.block_modal_state.prog_flow = (GCODE_MODAL_PROG_FLOW_MODES)work_var;
                        success_bits = MODAL_GROUP_M4_BIT;
                        break;
                    
                    case MODAL_SPINDLE_CW:      // M3, M4, M5 Spindle control
                    case MODAL_SPINDLE_CCW:
                    case MODAL_SPINDLE_OFF: 
                        m_block_data.block_modal_state.spindle_mode = (GCODE_MODAL_SPINDLE_MODES)work_var;
                        success_bits = MODAL_GROUP_M7_BIT;
                        break;
                    
                    case MODAL_SPINDLE_TOOL_CHANGE: // M6
                        success_bits = MODAL_GROUP_M6_BIT;
                        break;

                    case MODAL_COOLANT_MIST:    // M7, M8, M9
                    case MODAL_COOLANT_FLOOD:
                    case MODAL_COOLANT_OFF:
                        m_block_data.block_modal_state.coolant_mode = (GCODE_MODAL_COOLANT_MODES)work_var;
                        success_bits = MODAL_GROUP_M8_BIT;
                        break;
                    
                    // Testing [Values must be specified before M32 & M36]
                    case 32:    // M32 update speeds [max rate mm/min]
                    {
                        float fval;
                        
                        if ((m_value_group_flags & VALUE_SET_X_BIT) != 0)
                        {
                            // Values specified in mm/min -> convert to mm/sec
                            fval = m_block_data.coordinate_data[COORD_X] / 60.0f;
                            Settings_Manager::SetMaxSpeed_mm_sec_axis(COORD_X, fval);
                        }
                        
                        if ((m_value_group_flags & VALUE_SET_Y_BIT) != 0)
                        {
                            fval = m_block_data.coordinate_data[COORD_Y] / 60.0f;
                            Settings_Manager::SetMaxSpeed_mm_sec_axis(COORD_Y, fval);
                        }
                        
                        if ((m_value_group_flags & VALUE_SET_Z_BIT) != 0)
                        {
                            fval = m_block_data.coordinate_data[COORD_Z] / 60.0f;
                            Settings_Manager::SetMaxSpeed_mm_sec_axis(COORD_Z, fval);
                        }
                    }
                    break;
                    
                    case 36:    // M36 [Update accelerations mm/min2]
                    {
                        float fval;
                        
                        // Values specified in mm/min2 -> convert to mm/sec2 (div 3600)
                        
                        if ((m_value_group_flags & VALUE_SET_X_BIT) != 0)
                        {
                            fval = m_block_data.coordinate_data[COORD_X] / 3600.0f;
                            Settings_Manager::SetAcceleration_mm_sec2_axis(COORD_X, fval);
                        }
                        
                        if ((m_value_group_flags & VALUE_SET_Y_BIT) != 0)
                        {
                            fval = m_block_data.coordinate_data[COORD_Y] / 3600.0f;
                            Settings_Manager::SetAcceleration_mm_sec2_axis(COORD_Y, fval);
                        }
                        
                        if ((m_value_group_flags & VALUE_SET_Z_BIT) != 0)
                        {
                            fval = m_block_data.coordinate_data[COORD_Z] / 3600.0f;
                            Settings_Manager::SetAcceleration_mm_sec2_axis(COORD_Z, fval);
                        }
                    }
                    break;
                    
                    case 38:   // M38 Save settings to flash
                    {
                        Settings_Manager::Save();
                    }
                    break;
                    
                    default:
                    {
                        // Should not arrive here.
                        return GCODE_ERROR_INVALID_M_CODE;
                    }
                }
                
                // Check for mode violations
                if ((modal_group_flags & success_bits) != 0)
                {
                    // Another code from this modal group has been already
                    // defined. Flag this violation
                    return GCODE_ERROR_MODAL_G_GROUP_VIOLATION;
                }
                
                // Update group definition bits
                modal_group_flags |= success_bits;
			}
			break;
            
            case 'g':
			{
                float int_part, mant;

				// read G code                
                // Try to read float value
                value = DataConverter::StringToFloat(&m_line, &success_bits);
                
                // G codes must be positive
                if (value < 0.0f || success_bits == 0)
                    return GCODE_ERROR_INVALID_G_CODE;
                
                // Convert to integer (x 10)
                int_part = truncf(value);
				mant = roundf(10.0f * (value - int_part));

				work_var = (int16_t)(int_part * 10.0f + mant);
                
                // Parse specific codes
                switch (work_var)
                {
                    // Non-modal G Codes that can contain axis words [Group 0]
                    // ----------------------------------------------------------------------------
                    case NON_MODAL_SET_COORDINATE_DATA:     // G10
                    case NON_MODAL_GO_HOME_0:               // G28
                    case NON_MODAL_GO_HOME_1:               // G30
                    case NON_MODAL_SET_COORDINATE_OFFSET_SAVE:   // G92
                    {
                        if (m_axis_command_type != AXIS_COMMAND_TYPE_NONE)
                            return GCODE_ERROR_MODAL_AXIS_CONFLICT;
                        
                        // Signal the presence of a non-modal command
                        m_axis_command_type = AXIS_COMMAND_TYPE_NON_MODAL;
                    }
                        
                    // NO BREAK INTENTIONAL
                    case NON_MODAL_SET_HOME_0:              // G28.1
                    case NON_MODAL_SET_HOME_1:              // G30.1
                    case NON_MODAL_RESET_COORDINATE_OFFSET_SAVE: // G92.1
                    case NON_MODAL_RESET_COORDINATE_OFFSET_NO_SAVE: // G92.2
                    case NON_MODAL_APPLY_SAVED_COORDINATE_OFFSETS:  // G92.3
                        
                    case NON_MODAL_DWELL:                   // G4    
                    case NON_MODAL_ABSOLUTE_OVERRIDE:       // G53
                    {
                        // Update specific non-modal command
                        m_block_data.non_modal_code = work_var; // Any of the previous cases
                        success_bits = MODAL_GROUP_G0_BIT;
                    }
                    break;
                    
                    // Motion G Codes [Group 1]
                    // ----------------------------------------------------------------------------
                    case MODAL_MOTION_MODE_SEEK:            // G0
                    case MODAL_MOTION_MODE_LINEAR_FEED:     // G1
                    case MODAL_MOTION_MODE_HELICAL_CW:      // G2
                    case MODAL_MOTION_MODE_HELICAL_CCW:     // G3
                    case MODAL_MOTION_MODE_PROBE:           // G38.2
                    {
                        if (m_axis_command_type != AXIS_COMMAND_TYPE_NONE)
                            return GCODE_ERROR_MODAL_AXIS_CONFLICT;
                        
                        m_axis_command_type = AXIS_COMMAND_TYPE_MOTION;
                    }
                    
                    // NO BREAK INTENTIONAL
                    case MODAL_MOTION_MODE_CANCEL_MOTION:   // G80
                    {
                        m_block_data.block_modal_state.motion_mode = (GCODE_MODAL_MOTION_MODES)work_var;    // Any previous cases
                        success_bits = MODAL_GROUP_G1_BIT;
                    }
                    break;
                    
                    // Plane Selection G Codes [Group 2]
                    // ----------------------------------------------------------------------------
                    case MODAL_PLANE_SELECT_XY:   // G17
                    case MODAL_PLANE_SELECT_XZ:   // G18
                    case MODAL_PLANE_SELECT_YZ:   // G19
                    {
                        m_block_data.block_modal_state.plane_select = (GCODE_MODAL_PLANE_SELECT_MODES)work_var;
                        success_bits = MODAL_GROUP_G2_BIT;
                    }
                    break;
                    
                    // Distance Mode G Codes [Group 3]
                    // ----------------------------------------------------------------------------
                    case MODAL_DISTANCE_MODE_ABSOLUTE:      // G90
                    case MODAL_DISTANCE_MODE_INCREMENTAL:   // G91
                    {
                        m_block_data.block_modal_state.distance_mode = (GCODE_MODAL_DISTANCE_MODES)work_var;
                        success_bits = MODAL_GROUP_G3_BIT;
                    }
                    break;
                    
                    // Feed Rate Mode G Codes [Group 5]
                    // ----------------------------------------------------------------------------
                    case MODAL_FEEDRATE_MODE_UNITS_PER_MIN: // G94
                    case MODAL_FEEDRATE_MODE_INVERSE_TIME:  // G93
                    {
                        m_block_data.block_modal_state.feedrate_mode = (GCODE_MODAL_FEEDRATE_MODES)work_var;
                        success_bits = MODAL_GROUP_G5_BIT;
                    }
                    break;
                    
                    // Units Mode G Codes [Group 6]
                    // ----------------------------------------------------------------------------
                    case MODAL_UNITS_MODE_INCHES:           // G20
                    case MODAL_UNITS_MODE_MILLIMETERS:      // G21
                    {
                        m_block_data.block_modal_state.units_mode = (GCODE_MODAL_UNITS_MODES)work_var;
                        success_bits = MODAL_GROUP_G6_BIT;
                    }
                    break;

                    // Cutter Radius Compensation G Codes [Group 7]
                    // ----------------------------------------------------------------------------
                    case MODAL_CUTTER_RAD_COMP_OFF:         // G40
                    case MODAL_CUTTER_RAD_COMP_LEFT:        // G41
                    case MODAL_CUTTER_RAD_COMP_RIGHT:       // G42
                    {
						// Check for any G54, ..., G59.3 codes specified in this block
						if ((modal_group_flags & MODAL_GROUP_G12_BIT) != 0)
							return GCODE_ERROR_CANNOT_CHANGE_WCS_WITH_RAD_COMP;

                        m_block_data.block_modal_state.cutter_comp_mode = (GCODE_MODAL_CUTTER_RAD_COMP_MODES)work_var;
                        success_bits = MODAL_GROUP_G7_BIT;
                    }
                    break;
                    
                    // Tool Lenght Offset G Codes [Group 8]
                    // ----------------------------------------------------------------------------
                    //case MODAL_TOOL_LEN_OFS_PLUS:         // G43
                    case MODAL_TOOL_LEN_OFS_CANCEL:         // G49
                    {
                        if (m_axis_command_type != AXIS_COMMAND_TYPE_NONE)
                            return GCODE_ERROR_MODAL_AXIS_CONFLICT;
                        
                        m_axis_command_type = AXIS_COMMAND_TYPE_TOOL_LEN_OFS;
                        m_block_data.block_modal_state.tool_len_ofs_mode = (GCODE_MODAL_TOOL_LEN_OFS_MODES)work_var;
                        success_bits = MODAL_GROUP_G8_BIT;
                    }
                    break;
                    
                    // Coordinate System Selection G Codes [Group 12]
                    // ----------------------------------------------------------------------------
                    case MODAL_COORD_SYS_SELECT_1:          // G54      index = 0
                    case MODAL_COORD_SYS_SELECT_2:          // G55
                    case MODAL_COORD_SYS_SELECT_3:          // G56        
                    case MODAL_COORD_SYS_SELECT_4:          // G57
                    case MODAL_COORD_SYS_SELECT_5:          // G58
                    case MODAL_COORD_SYS_SELECT_6:          // G59      index = 5
                    case MODAL_COORD_SYS_SELECT_7:          // G59.1    index = 6
                    case MODAL_COORD_SYS_SELECT_8:          // G59.2
                    case MODAL_COORD_SYS_SELECT_9:          // G59.3    index = 8
                    {
						// Check if currently Cutter radius compensation is on
						if (m_block_data.block_modal_state.cutter_comp_mode != MODAL_CUTTER_RAD_COMP_OFF)
							return GCODE_ERROR_CANNOT_CHANGE_WCS_WITH_RAD_COMP;

						// Convert GCODE_MODAL_COORD_SYS_SELECT_MODES to integer index
						if (work_var > MODAL_COORD_SYS_SELECT_6)
                            m_block_data.block_modal_state.work_coord_sys_index = ((work_var - MODAL_COORD_SYS_SELECT_7) + 6);
						else
                            m_block_data.block_modal_state.work_coord_sys_index = ((work_var - MODAL_COORD_SYS_SELECT_1)/10);
						
                        success_bits = MODAL_GROUP_G12_BIT;                        
                    }
                    break;
                    
                    // Path Mode (only G61 supported) [Group 13]
                    // ----------------------------------------------------------------------------
                    case MODAL_PATH_MODE_EXACT_PATH:        // G61
                    //case MODAL_PATH_MODE_EXACT_STOP:        // G61.1
                    //case MODAL_PATH_MODE_CONTINUOUS:        // G64
                        success_bits = MODAL_GROUP_G13_BIT;
                        break;

                    // Canned Cycles
                    case MODAL_MOTION_MODE_CANNED_DRILL_G81:            // G81
                    case MODAL_MOTION_MODE_CANNED_DRILL_DWELL_G82:      // G82
                    case MODAL_MOTION_MODE_CANNED_DRILL_PECK_G83:       // G83
//                    case MODAL_MOTION_MODE_CANNED_TAPPING_G84:          // G84
//                    case MODAL_MOTION_MODE_CANNED_BORING_G85:           // G85
//                    case MODAL_MOTION_MODE_CANNED_BORING_DWELL_G86:     // G86
//                    case MODAL_MOTION_MODE_CANNED_COUNTERBORING_G87:    // G87
//                    case MODAL_MOTION_MODE_CANNED_BORING_G88:           // G88
//                    case MODAL_MOTION_MODE_CANNED_BORING_G89:           // G89
                    {
                        if (m_axis_command_type != AXIS_COMMAND_TYPE_NONE)
                            return GCODE_ERROR_MODAL_AXIS_CONFLICT;
                        
                        m_axis_command_type = AXIS_COMMAND_TYPE_CANNED_CYCLE;

                        m_block_data.block_modal_state.motion_mode = (GCODE_MODAL_MOTION_MODES)work_var;
                        success_bits = MODAL_GROUP_G1_BIT;
                    }
                    break;

                    // Canned cycle return mode
                    case MODAL_CANNED_RETURN_TO_PREV_POSITION:
                    case MODAL_CANNED_RETURN_TO_R_POSITION:
                    {
                        m_block_data.block_modal_state.canned_return_mode = (GCODE_MODAL_CANNED_RETURN_MODES)work_var;
                        success_bits |= MODAL_GROUP_G10_BIT;
                    }
                    break;

                    default:
                        return GCODE_ERROR_UNSUPPORTED_CODE_FOUND;
                }
                
                // Check for mode violations
                if ((modal_group_flags & success_bits) != 0)
                {
                    // Another code from this modal group has been already
                    // defined. Flag this violation
                    return GCODE_ERROR_MODAL_G_GROUP_VIOLATION;
                }
                
                // Update group definition bits
                modal_group_flags |= success_bits;
            }
			break;
            
            default:
                return GCODE_ERROR_UNSUPPORTED_CODE_FOUND;

        }   //  end of 'switch (ch)'
    }   // end 'while ( ... )'

    // Perform some checkings prior to execution of codes
    work_var = check_codes_using_axes();

	if (work_var != GCODE_OK)
	{
		// Reset motion mode as errors happened
		m_parser_modal_state.motion_mode = MODAL_MOTION_MODE_CANCEL_MOTION;
		return work_var;
	}
    
    work_var = check_group_0_codes();

    if (work_var != GCODE_OK)
        return work_var;

    work_var = check_unused_codes();

    if (work_var != GCODE_OK)
        return work_var;

    // Try to execute the code of this line

    // Update Parser Status Values:

    /// 1 - Update Feed Rate Mode ///
    m_parser_modal_state.feedrate_mode = m_block_data.block_modal_state.feedrate_mode;

    /// 2 - Update Feed Rate Value (Only in units/min mode G94) ///
    if (m_parser_modal_state.feedrate_mode == MODAL_FEEDRATE_MODE_UNITS_PER_MIN)
        m_feed_rate = m_block_data.feed_rate;

    /// 3 - Update Spindle Speed Value ///
    m_spindle_speed = m_block_data.spindle_speed;

    /// 4 - Select Active Tool Number ///
    m_tool_number = m_block_data.tool_number;

    /// 5 - Handle a possible Tool Change [M6 Code] ///
    if ((modal_group_flags & MODAL_GROUP_M6_BIT) != 0)
    {
        // Wait for idle condition before attempt to stop spindle for tool change
        work_var = machine->WaitForIdleCondition();
        
        if (work_var != GCODE_OK)
            return work_var;
        
        // Stop Spindle and leave coolant as currently is 
        work_var = machine->SendSpindleCommand(MODAL_SPINDLE_OFF, 0.0f);
        
        if (work_var != GCODE_OK)
            return work_var;
    }

    /// 6 - Handle Spindle Control Commands [M3, M4, M5] ///
    if (m_parser_modal_state.spindle_mode != m_block_data.block_modal_state.spindle_mode)
    {
        m_parser_modal_state.spindle_mode = m_block_data.block_modal_state.spindle_mode;

        // Wait for idle condition before attempt to change spindle operation
        work_var = machine->WaitForIdleCondition();
        
        if (work_var != GCODE_OK)
            return work_var;
        
        // Notify of spindle changes
        work_var = machine->SendSpindleCommand(m_parser_modal_state.spindle_mode, m_spindle_speed);
        
        if (work_var != GCODE_OK)
            return work_var;
    }

    /// 7 - Handle Coolant Control Commands [M7, M8, M9] ///
    if (m_parser_modal_state.coolant_mode != m_block_data.block_modal_state.coolant_mode)
    {
        m_parser_modal_state.coolant_mode = m_block_data.block_modal_state.coolant_mode;

        // Wait for idle condition before attempt to change coolant operation
        work_var = machine->WaitForIdleCondition();
        
        if (work_var != GCODE_OK)
            return work_var;
        
        // Notify of coolant changes
        work_var = machine->SendCoolantCommand(m_parser_modal_state.coolant_mode);
        
        if (work_var != GCODE_OK)
            return work_var;
    }

    /// 8 - Handle Speed/Feed Overrides [M48, M49] ///


    /// 9 - Handle Dwell Command [G4] ///
    if (m_block_data.non_modal_code == NON_MODAL_DWELL) // G4
    {
        // NOTE: Before executing dwell operations we must wait for the completion of previous 
        // queued commands
        
        // Wait for idle condition before attempt to enter dwell mode
        work_var = machine->WaitForIdleCondition();
        
        if (work_var != GCODE_OK)
            return work_var;
        
        work_var = machine->Dwell(m_block_data.P_value);
        
        if (work_var != GCODE_OK)
            return work_var;
    }

    /// 10 - Handle Plane Selection Commands [G17, G18, G19] ///
	if (m_parser_modal_state.plane_select != m_block_data.block_modal_state.plane_select)
	{
		m_parser_modal_state.plane_select = m_block_data.block_modal_state.plane_select;

		switch (m_parser_modal_state.plane_select)
		{
			case MODAL_PLANE_SELECT_XY: 
			{ 
				m_axis_zero = COORD_X; 
				m_axis_one = COORD_Y;
				m_axis_linear = COORD_Z;
			}
			break;

			case MODAL_PLANE_SELECT_XZ:
			{
				m_axis_zero = COORD_X;
				m_axis_one = COORD_Z;
				m_axis_linear = COORD_Y;
			}
			break;

			default:	// case MODAL_PLANE_SELECT_YZ:
			{
				m_axis_zero = COORD_Y;
				m_axis_one = COORD_Z;
				m_axis_linear = COORD_X;
			}
			break;
		}
	}

    /// 11 - Handle Length Units Selection Commands [G20, G21] ///
    /// Internally always use millimeters
    m_parser_modal_state.units_mode = m_block_data.block_modal_state.units_mode;

    /// 12 - Handle Cutter Radius Compensation Commands [G40, G41, G42] ///
    m_parser_modal_state.cutter_comp_mode = m_block_data.block_modal_state.cutter_comp_mode;
    /// TODO: Implement this function ///

    /// 13 - Handle Tool Length Offset Commands [G43, G49] ///
    m_parser_modal_state.tool_len_ofs_mode = m_block_data.block_modal_state.tool_len_ofs_mode;

    /// 14 - Handle Coordinate System Selection Commands [G54, G55, ...] ///
    handle_coordinate_system_select();
    
    /// 15 - Handle Control Mode Commands [G61, G61.1, G64] ///
    

    /// 16 - Handle Distance Mode Commands [G90, G91] ///
    m_parser_modal_state.distance_mode = m_block_data.block_modal_state.distance_mode;

    /// 17 - Handle Retract Mode Commands [G98, G99]
    m_parser_modal_state.canned_return_mode = m_block_data.block_modal_state.canned_return_mode;
    
    // Check if specified either G98/G99 to start a canned cycle
    if ((modal_group_flags & MODAL_GROUP_G10_BIT) != 0)
    {
        // Wait for idle condition
        work_var = machine->WaitForIdleCondition();
        
        if (work_var != GCODE_OK)
            return work_var;
        
        // Update initial Z value [in Work Coord System]
        m_cc_initial_z = m_gcode_machine_pos[COORD_Z] - m_work_coord_sys[COORD_Z] + m_g92_coord_offset[COORD_Z] - m_tool_offset[COORD_Z];
        
        // Reset sticky values
        canned_cycle_reset_stycky();
        
        m_canned_cycle_active = true;
    }

    /// 18 - Handle Non-Modal Commands [G10, G28, G30, G92] ///
    if (m_block_data.non_modal_code != 0)
    {
        work_var = handle_non_modal_codes();

        if (work_var != GCODE_OK)
            return work_var;
    }

    /// 19 - Execute Motion Commands [G0, G1, G2, G3, G38, G81-G89] ///
    if ((m_axis_command_type == AXIS_COMMAND_TYPE_MOTION) ||
		(m_axis_command_type == AXIS_COMMAND_TYPE_CANNED_CYCLE))
    {		
        work_var = handle_motion_commands();

        if (work_var != GCODE_OK)
            return work_var;
    }
    
    /// 19.1 - Handle G80 stop command for canned cycles
    if ( (m_block_data.block_modal_state.motion_mode == MODAL_MOTION_MODE_CANCEL_MOTION) &&
         (m_canned_cycle_active != false) )
    {
        m_canned_cycle_active = false;
        
        if (m_parser_modal_state.canned_return_mode == MODAL_CANNED_RETURN_TO_R_POSITION)
        {
            // If retract mode is to R-plane -> Enqueue a last move to this position
        
            // TODO: Implement this
        }
    }

    /// 20 - Handle Stop Commands [M0, M1, M2, M30]
    if (m_parser_modal_state.prog_flow != m_block_data.block_modal_state.prog_flow)
    {
        // Change in program flow. Update parser modal state
        m_parser_modal_state.prog_flow = m_block_data.block_modal_state.prog_flow;
        
        // Before executing flow control operations we must wait for the completion of previous 
        // queued commands
        
        // Wait for idle condition before attempt to change flow mode
        work_var = machine->WaitForIdleCondition();
        
        if (work_var != GCODE_OK)
            return work_var;
        
        if (m_parser_modal_state.prog_flow == MODAL_FLOW_TEMPORARY_STOP)    // M0 [Feed hold]
        {
            if (m_check_mode != false)
            {
                // Notify to other sub-systems of feed hold condition
                machine->EnterFeedHold();
            }                
        }
        else if ( (m_parser_modal_state.prog_flow == MODAL_FLOW_COMPLETED) ||       // M2
                  (m_parser_modal_state.prog_flow == MODAL_FLOW_COMPLETED_PALLET))  // M30
        {
            // Program completed. Restore some defaults
            m_parser_modal_state.motion_mode = MODAL_MOTION_MODE_LINEAR_FEED;   // G1
            
            m_parser_modal_state.plane_select = MODAL_PLANE_SELECT_XY;
            m_axis_zero = COORD_X;
            m_axis_one = COORD_Y;
            m_axis_linear = COORD_Z;
            
            m_parser_modal_state.distance_mode = MODAL_DISTANCE_MODE_ABSOLUTE;
            m_parser_modal_state.feedrate_mode = MODAL_FEEDRATE_MODE_UNITS_PER_MIN;
            m_parser_modal_state.work_coord_sys_index = 0;
            m_parser_modal_state.spindle_mode = MODAL_SPINDLE_OFF;
            m_parser_modal_state.coolant_mode = MODAL_COOLANT_OFF;
            
            handle_coordinate_system_select();
            
            if (m_check_mode == false)
            {
                // Notify of spindle changes
                work_var = machine->SendSpindleCommand(m_parser_modal_state.spindle_mode, m_spindle_speed);
                
                if (work_var != GCODE_OK)
                    return work_var;
                
                // Notify of coolant changes
                work_var = machine->SendCoolantCommand(m_parser_modal_state.coolant_mode);
                
                if (work_var != GCODE_OK)
                    return work_var;
            }
            
            machine->ClearHalt();
        }
    }
    
    // Reset program flow mode to Default running
    m_parser_modal_state.prog_flow = MODAL_FLOW_DEFAULT_RUNNING;
    return GCODE_OK;
}


int GCodeParser::cleanup_and_lowercase_line(uint32_t & line_len)
{
    char * rd_ptr = m_line;
	char * wr_ptr = m_line;
	bool inside_comment = false;
	char ch;

	while ((ch = *rd_ptr++) != '\0')
	{
		if (inside_comment != false)
		{
			if  (ch == ')')
				inside_comment = false;
			else if (ch == '(')
				return GCODE_ERROR_NESTED_COMMENT;

			continue;
		}
		else if (isspace(ch) != 0)
		    continue;
		else if (ch >= 'A' && ch <= 'Z')
		    ch += 0x20; // make lower case
		else if (ch == '(')
		{
			inside_comment = true;
			continue;
		}
		else if (ch == ')')
			return GCODE_ERROR_UNOPENED_COMMENT;

		*wr_ptr++ = ch;
	}

	if (inside_comment != false)
		return GCODE_ERROR_UNCLOSED_COMMENT;

	*wr_ptr = '\0';
    
    line_len = (uint32_t)(wr_ptr - m_line);
	return GCODE_OK;
}

void GCodeParser::reset_modal_params()
{
    // Set defaults for all modal parameters
    m_parser_modal_state.motion_mode				= MODAL_MOTION_MODE_CANCEL_MOTION;
    m_parser_modal_state.feedrate_mode				= MODAL_FEEDRATE_MODE_UNITS_PER_MIN;
    m_parser_modal_state.units_mode					= MODAL_UNITS_MODE_MILLIMETERS;
    m_parser_modal_state.distance_mode				= MODAL_DISTANCE_MODE_ABSOLUTE;
    m_parser_modal_state.plane_select				= MODAL_PLANE_SELECT_XY;
    m_parser_modal_state.prog_flow					= MODAL_FLOW_DEFAULT_RUNNING;
    m_parser_modal_state.coolant_mode				= MODAL_COOLANT_OFF;
    m_parser_modal_state.spindle_mode				= MODAL_SPINDLE_OFF;
    m_parser_modal_state.cutter_comp_mode			= MODAL_CUTTER_RAD_COMP_OFF;
    m_parser_modal_state.tool_len_ofs_mode			= MODAL_TOOL_LEN_OFS_CANCEL;
    m_parser_modal_state.work_coord_sys_index	    = 0;
    m_parser_modal_state.canned_return_mode         = MODAL_CANNED_RETURN_TO_R_POSITION; 
}

int GCodeParser::check_codes_using_axes()
{
    bool axis_presence = false;
    bool offsets_presence = false;
    bool mode_zero_uses_axes = false;

    // Flag the presence of any axis
    if ((m_value_group_flags & VALUE_SET_ANY_AXES_BITS) != 0)
        axis_presence = true;
    
    // Flag the presence of any offsets
    if ((m_value_group_flags & VALUE_SET_IJK_BITS) != 0)
        offsets_presence = true;

    if ( (m_block_data.non_modal_code == NON_MODAL_SET_COORDINATE_DATA) ||  // G10
         (m_block_data.non_modal_code == NON_MODAL_GO_HOME_0) ||            // G28
         (m_block_data.non_modal_code == NON_MODAL_GO_HOME_1) ||            // G30
         (m_block_data.non_modal_code == NON_MODAL_SET_COORDINATE_OFFSET_SAVE) ) // G92
    {
        mode_zero_uses_axes = true;
    }

    // Check if specified any code from Group 1 [Motion]
    if (m_axis_command_type == AXIS_COMMAND_TYPE_MOTION || 
        m_axis_command_type == AXIS_COMMAND_TYPE_CANNED_CYCLE)
    {
        // Check for G80 with axis specified (violation)
        if (m_block_data.block_modal_state.motion_mode == MODAL_MOTION_MODE_CANCEL_MOTION)
        {
            // Specified G80 with coordinates and not using any Group 0 code requesting axes
            if (axis_presence != false && mode_zero_uses_axes == false)
                return GCODE_ERROR_COORD_DATA_IN_G80_CODE;

            // Specified G80 with G92 and coordinates are missing
            if (axis_presence == false && m_block_data.non_modal_code == NON_MODAL_SET_COORDINATE_OFFSET_SAVE)
                return GCODE_ERROR_MISSING_XYZ_G92_CODE;
        }
        else
        {
            // Other motion codes [G0, G1, G2, G3, ...]
            // require coordinate data
            if (axis_presence == false)
                return GCODE_ERROR_MISSING_COORDS_IN_MOTION;

            // In the specific case of G2/G3 can be used either I,J,K offsets (center fmt arc) or R values 
            // (radius fmt arc) also check for conflicts using both offsets/R value
            if (m_block_data.block_modal_state.motion_mode == MODAL_MOTION_MODE_HELICAL_CW ||
                m_block_data.block_modal_state.motion_mode == MODAL_MOTION_MODE_HELICAL_CCW)
            {
                // G2/G3
                if ((m_value_group_flags & VALUE_SET_R_BIT) == 0 && offsets_presence == false)
                {
                    // Using G2/G3 without specifying R value nor IJK offsets
                    return GCODE_ERROR_MISSING_ARC_DATA_HELICAL_MOTION;
                }
                else if ((m_value_group_flags & VALUE_SET_R_BIT) != 0 && offsets_presence != false)
                {
                    // Using G2/G3 with R value and IJK offsets at the same time
                    return GCODE_ERROR_CONFLICT_ARC_DATA_HELICAL_MOTION;
                }

				// Check if at least one of the two words is in the active plane. 
				uint32_t check_val;
				uint32_t ofs_check;

				switch (m_block_data.block_modal_state.plane_select)
				{
					case MODAL_PLANE_SELECT_XY:
						check_val = m_value_group_flags & VALUE_SET_XY_BITS;
						ofs_check = m_value_group_flags & VALUE_SET_IJ_BITS;
						break;

					case MODAL_PLANE_SELECT_XZ:
						check_val = m_value_group_flags & VALUE_SET_XZ_BITS;
						ofs_check = m_value_group_flags & VALUE_SET_IK_BITS;
						break;

					default:
						check_val = m_value_group_flags & VALUE_SET_YZ_BITS;
						ofs_check = m_value_group_flags & VALUE_SET_JK_BITS;
						break;
				}

				if (check_val == 0)
				{
					// All the required words for active plane are missing.
					return GCODE_ERROR_MISSING_COORDS_IN_PLANE;
				}

				// In case of Center format arcs, check the presence of at least one offset in the same active plane
				if ((offsets_presence != false) && (ofs_check == 0))
				{
					// All required offsets words are missing for the active plane
					return GCODE_ERROR_MISSING_OFFSETS_IN_PLANE;
				}				
            }
        }
    }
    else if (mode_zero_uses_axes != false)
    {
        // In Group 0 only G92 requires axes information
        if ( (m_block_data.non_modal_code == NON_MODAL_SET_COORDINATE_OFFSET_SAVE) &&
             (axis_presence == false) )
        {
            return GCODE_ERROR_MISSING_XYZ_G92_CODE;
        }
    }
	else if (axis_presence != false)
	{
		// Check if using coordinate axes data without being
		// active any command to use it
		if (m_parser_modal_state.motion_mode == MODAL_MOTION_MODE_CANCEL_MOTION)
			return GCODE_ERROR_USING_COORDS_NO_MOTION_MODE;

		//// Check if using coordinates without using G0/G1/G2/G3 ////
		if (m_axis_command_type == AXIS_COMMAND_TYPE_NONE)
		{
			// If using coordinates but not specifying G0/G1/G2/G3 and G0/G1/G2/G3 were the previous modes
			// then continue to use them
			if (m_parser_modal_state.motion_mode <= MODAL_MOTION_MODE_HELICAL_CCW)
			{
				m_axis_command_type = AXIS_COMMAND_TYPE_MOTION;
			}
			else
			{
				// Not G0/G1/G2/G3. Flag error
				return GCODE_ERROR_USING_COORDS_NO_MOTION_MODE;
			}
		}
    }

	// Check for invalid feedrate for G1/G2/G3, G81/G82/G83/G84/G85/G86/G87/G88/G89. Exclude possible homing commands (G28/G30)
	if (((m_block_data.block_modal_state.motion_mode > MODAL_MOTION_MODE_SEEK) &&
		(m_block_data.block_modal_state.motion_mode < MODAL_MOTION_MODE_CANCEL_MOTION)) ||
		((m_block_data.block_modal_state.motion_mode >= MODAL_MOTION_MODE_CANNED_DRILL_G81) &&
		(m_block_data.block_modal_state.motion_mode <= MODAL_MOTION_MODE_CANNED_DRILL_PECK_G83)))
	{
		if ((m_block_data.feed_rate == 0.0f) && (m_feed_rate == 0.0f) && 
            ((m_axis_command_type == AXIS_COMMAND_TYPE_MOTION) ||
             (m_axis_command_type == AXIS_COMMAND_TYPE_CANNED_CYCLE)))
		{
			// Feed rate not specified or invalid.
			return GCODE_ERROR_INVALID_FEEDRATE_VALUE;
		}

		// Check if Inverse time feedrate mode and feed rate not specified in this block
		if ((m_block_data.block_modal_state.feedrate_mode == MODAL_FEEDRATE_MODE_INVERSE_TIME) &&
			((m_value_group_flags & VALUE_SET_F_BIT) == 0) && 
            ((m_axis_command_type == AXIS_COMMAND_TYPE_MOTION) || 
             (m_axis_command_type == AXIS_COMMAND_TYPE_CANNED_CYCLE)))
		{
			// Feed rate missing using inverse time feed rate move with G1/G2/G3 ...
			return GCODE_ERROR_MISSING_FEEDRATE_INVERSE_TIME_MODE;
		}
	}

    return GCODE_OK;
}
 
int GCodeParser::check_group_0_codes()
{
    if (m_block_data.non_modal_code != 0)
    {
        switch (m_block_data.non_modal_code)
        {
            case NON_MODAL_DWELL:                   // G4
            {
                if ((m_value_group_flags & VALUE_SET_P_BIT) == 0)
                    return GCODE_ERROR_MISSING_DWELL_TIME;
            }
            break;

            case NON_MODAL_SET_COORDINATE_DATA:     // G10
            {
                int32_t P_int = (int32_t)m_block_data.P_value;

                // Check for L index == 2
                if (m_block_data.L_value != 2)
                    return GCODE_ERROR_INVALID_L_VALUE;

                // Check for P value (1 <= P <= 9)
                if ((P_int < 1) || (P_int > 9))
                    return GCODE_ERROR_INVALID_P_VALUE;
            }
            break;

            case NON_MODAL_GO_HOME_0:               // G28
            case NON_MODAL_GO_HOME_1:               // G30
            case NON_MODAL_SET_COORDINATE_OFFSET_SAVE:   // G92
            case NON_MODAL_RESET_COORDINATE_OFFSET_SAVE: // G92.1
            case NON_MODAL_RESET_COORDINATE_OFFSET_NO_SAVE: // G92.2
            case NON_MODAL_APPLY_SAVED_COORDINATE_OFFSETS:  // G92.3
                break;

            case NON_MODAL_ABSOLUTE_OVERRIDE:       // G53
            {
                // G53 can only be used with G0/G1
                if ( (m_block_data.block_modal_state.motion_mode != MODAL_MOTION_MODE_SEEK) &&
                     (m_block_data.block_modal_state.motion_mode != MODAL_MOTION_MODE_LINEAR_FEED))
                {
                    return GCODE_ERROR_INVALID_G53_MOTION_MODE;
                }

                // G53 cannot be used in incremental mode
                if (m_block_data.block_modal_state.distance_mode != MODAL_DISTANCE_MODE_ABSOLUTE)
                    return GCODE_ERROR_INVALID_G53_DISTANCE_MODE;

				// G53 cannot be used while cutter radius compensation is on
				if (m_block_data.block_modal_state.cutter_comp_mode != MODAL_CUTTER_RAD_COMP_OFF)
					return GCODE_ERROR_ABSOLUTE_OVERRIDE_CANNOT_USE_RAD_COMP;
            }
            break;

            default:
                return GCODE_ERROR_INVALID_NON_MODAL_COMMAND;
        }
    }
    
    return GCODE_OK;
}

int GCodeParser::check_unused_codes()
{
    // Check the use of D value outside of G41/G42 [Cutter rad comp.]
    if ( ((m_value_group_flags & VALUE_SET_D_BIT) != 0) &&
        (m_block_data.block_modal_state.cutter_comp_mode == MODAL_CUTTER_RAD_COMP_OFF))
    {
        return GCODE_ERROR_USING_D_WORD_OUTSIDE_G41_G42;
    }

    // Check the use of H value outside of G43
    if ( ((m_value_group_flags & VALUE_SET_H_BIT) != 0) &&
         (m_block_data.block_modal_state.tool_len_ofs_mode == MODAL_TOOL_LEN_OFS_PLUS))
    {
        return GCODE_ERROR_USING_H_WORD_OUTSIDE_G43;
    }

    // Check the use of any I, J, K values outside of G2/G3
    if ( ((m_value_group_flags & VALUE_SET_IJK_BITS) != 0) &&
        (m_block_data.block_modal_state.motion_mode != MODAL_MOTION_MODE_HELICAL_CW) && 
        (m_block_data.block_modal_state.motion_mode != MODAL_MOTION_MODE_HELICAL_CCW) )
    {
        return GCODE_ERROR_UNUSED_OFFSET_VALUE_WORDS;
    }

    // Check the use of L word outside G10 or canned cycles
    if ( ((m_value_group_flags & VALUE_SET_L_BIT) != 0) &&
        (m_block_data.non_modal_code != NON_MODAL_SET_COORDINATE_DATA) &&   // Not G10
        (m_axis_command_type != AXIS_COMMAND_TYPE_CANNED_CYCLE) )           // Not in [G81 .. G89]
    {
        return GCODE_ERROR_UNUSED_L_VALUE_WORD;
    }

    // Check the use of P word outside G4, G10 or canned cycles [G82, G86, G88, G89]
    if (((m_value_group_flags & VALUE_SET_P_BIT) != 0) &&
        (m_block_data.non_modal_code != NON_MODAL_DWELL) &&                 // Not G4
        (m_block_data.non_modal_code != NON_MODAL_SET_COORDINATE_DATA) &&   // Not G10
        
        (m_block_data.block_modal_state.motion_mode != MODAL_MOTION_MODE_CANNED_DRILL_DWELL_G82)) // Not [G82, G86, G88, G89]
    {
        return GCODE_ERROR_UNUSED_P_VALUE_WORD;
    }

    // Check Q word used outside of G83 canned cycles
    if (((m_value_group_flags & VALUE_SET_Q_BIT) != 0) &&
        (m_block_data.block_modal_state.motion_mode != MODAL_MOTION_MODE_CANNED_DRILL_PECK_G83))
    {
        return GCODE_ERROR_UNUSED_Q_VALUE_WORD;
    }

    // Check the use of R word outside of G2/G3 or canned cycles
    if (((m_value_group_flags & VALUE_SET_R_BIT) != 0) &&
        (m_block_data.block_modal_state.motion_mode != MODAL_MOTION_MODE_HELICAL_CW) &&
        (m_block_data.block_modal_state.motion_mode != MODAL_MOTION_MODE_HELICAL_CCW) &&
        (m_axis_command_type != AXIS_COMMAND_TYPE_CANNED_CYCLE) )
    {
        return GCODE_ERROR_UNUSED_R_VALUE_WORD;
    }

    return GCODE_OK;
}

void GCodeParser::handle_coordinate_system_select()
{
    // Apply the selected coordinate system [G54, ..., G59.3]
    if (m_block_data.block_modal_state.work_coord_sys_index != m_parser_modal_state.work_coord_sys_index)
    {
        // Changed the active working coordinate system
        m_parser_modal_state.work_coord_sys_index = m_block_data.block_modal_state.work_coord_sys_index;

        Settings_Manager::ReadCoordinateValues(m_parser_modal_state.work_coord_sys_index, &m_work_coord_sys[0]);
        Settings_Manager::SetActiveCoordinateSystemIndex(m_parser_modal_state.work_coord_sys_index);
    }
}

int GCodeParser::handle_non_modal_codes()
{
    bool isG28 = false;
    int work_var = GCODE_OK;

    switch (m_block_data.non_modal_code)
    {
        case NON_MODAL_SET_COORDINATE_DATA: // G10
        {
            // Previously checked the presence of coordinate values (check_codes_using_axes),
            // also checked the values for L and P parameters in (check_group_0_codes)
            uint32_t P_int;
            float values[TOTAL_AXES_COUNT];

            P_int = (uint32_t)m_block_data.P_value;

			if ((m_value_group_flags & VALUE_SET_ANY_AXES_BITS) == 0)
			{
				// Don't waste time. Coordinates X, Y, Z, A, B, C are missing.
				break;
			}

            Settings_Manager::ReadCoordinateValues(P_int - 1, &values[0]);

            // Only modify specified values
            if (m_value_group_flags & VALUE_SET_X_BIT)
                values[COORD_X] = this->convert_to_mm(m_block_data.coordinate_data[COORD_X]);

            if (m_value_group_flags & VALUE_SET_Y_BIT)
                values[COORD_Y] = this->convert_to_mm(m_block_data.coordinate_data[COORD_Y]);

            if (m_value_group_flags & VALUE_SET_Z_BIT)
                values[COORD_Z] = this->convert_to_mm(m_block_data.coordinate_data[COORD_Z]);

            if (m_value_group_flags & VALUE_SET_A_BIT)
                values[COORD_A] = m_block_data.coordinate_data[COORD_A];

            if (m_value_group_flags & VALUE_SET_B_BIT)
                values[COORD_B] = m_block_data.coordinate_data[COORD_B];

            if (m_value_group_flags & VALUE_SET_C_BIT)
                values[COORD_C] = m_block_data.coordinate_data[COORD_C];

            // Update settings
            Settings_Manager::WriteCoordinateValues(P_int - 1, &values[0]);

            // Check if active to update
            if (m_parser_modal_state.work_coord_sys_index == (P_int - 1))
                memcpy(&m_work_coord_sys[0], &values[0], sizeof(m_work_coord_sys));
        }
        break;

        case NON_MODAL_GO_HOME_0:           // G28
            isG28 = true;

            // NO BREAK
        case NON_MODAL_GO_HOME_1:           // G30
        {
            float values[TOTAL_AXES_COUNT];
            uint32_t index;
            
            // Wait for idle condition before attempt to perform any homing command
            work_var = machine->WaitForIdleCondition();
            
            if (work_var != GCODE_OK)
                return work_var;
            
            for (index = COORD_X; index < TOTAL_AXES_COUNT; index++)
            {
                if (index < COORD_A)
                    values[index] = this->convert_to_mm(m_block_data.coordinate_data[index]);
                else
                    values[index] = m_block_data.coordinate_data[index];
            }
            
            work_var = machine->GoHome(&values[0], m_value_group_flags, isG28);
            
            if (work_var != GCODE_OK)
                return work_var;
        }
        break;

        case NON_MODAL_SET_HOME_0:          // G28.1
            isG28 = true;

            // NO BREAK
        case NON_MODAL_SET_HOME_1:          // G30.1
        {
            float values[TOTAL_AXES_COUNT];

            // Read original G28/G30 values
            Settings_Manager::ReadCoordinateValues((isG28 ? 28 : 30), &values[0]);

            // Update only specified values
            if (m_value_group_flags & VALUE_SET_X_BIT)
                values[COORD_X] = convert_to_mm(m_block_data.coordinate_data[COORD_X]);

            if (m_value_group_flags & VALUE_SET_Y_BIT)
                values[COORD_Y] = convert_to_mm(m_block_data.coordinate_data[COORD_Y]);

            if (m_value_group_flags & VALUE_SET_Z_BIT)
                values[COORD_Z] = convert_to_mm(m_block_data.coordinate_data[COORD_Z]);

            if (m_value_group_flags & VALUE_SET_A_BIT)
                values[COORD_A] = m_block_data.coordinate_data[COORD_A];

            if (m_value_group_flags & VALUE_SET_B_BIT)
                values[COORD_B] = m_block_data.coordinate_data[COORD_B];

            if (m_value_group_flags & VALUE_SET_C_BIT)
                values[COORD_C] = m_block_data.coordinate_data[COORD_C];

            // Write back new values to G28/G30 positions
            Settings_Manager::WriteCoordinateValues((isG28 ? 28 : 30), &values[0]);
        }
        break;

        case NON_MODAL_SET_COORDINATE_OFFSET_SAVE:          // G92
        case NON_MODAL_RESET_COORDINATE_OFFSET_SAVE:        // G92.1
        case NON_MODAL_RESET_COORDINATE_OFFSET_NO_SAVE:     // G92.2
        case NON_MODAL_APPLY_SAVED_COORDINATE_OFFSETS:      // G92.3
        {
            float values[TOTAL_AXES_COUNT];
            uint32_t action = m_block_data.non_modal_code - NON_MODAL_SET_COORDINATE_OFFSET_SAVE;

            switch (action)
            {
                case 0:     // G92 [Set coordinate offsets and save to parameters]
                {   
                    // Read current G92 offsets
                    memcpy((void*)&values[0], m_g92_coord_offset, sizeof(values));

                    // Update specified values
                    if (m_value_group_flags & VALUE_SET_X_BIT)
                        values[COORD_X] = m_gcode_machine_pos[COORD_X] - m_work_coord_sys[COORD_X] - convert_to_mm(m_block_data.coordinate_data[COORD_X]) - m_tool_offset[COORD_X];

                    if (m_value_group_flags & VALUE_SET_Y_BIT)
                        values[COORD_Y] = m_gcode_machine_pos[COORD_Y] - m_work_coord_sys[COORD_Y] - convert_to_mm(m_block_data.coordinate_data[COORD_Y]) - m_tool_offset[COORD_Y];

                    if (m_value_group_flags & VALUE_SET_Z_BIT)
                        values[COORD_Z] = m_gcode_machine_pos[COORD_Z] - m_work_coord_sys[COORD_Z] - convert_to_mm(m_block_data.coordinate_data[COORD_Z]) - m_tool_offset[COORD_Z];

                    if (m_value_group_flags & VALUE_SET_A_BIT)
                        values[COORD_A] = m_gcode_machine_pos[COORD_A] - m_work_coord_sys[COORD_A] - m_block_data.coordinate_data[COORD_A];

                    if (m_value_group_flags & VALUE_SET_B_BIT)
                        values[COORD_B] = m_gcode_machine_pos[COORD_B] - m_work_coord_sys[COORD_B] - m_block_data.coordinate_data[COORD_B];

                    if (m_value_group_flags & VALUE_SET_C_BIT)
                        values[COORD_C] = m_gcode_machine_pos[COORD_C] - m_work_coord_sys[COORD_C] - m_block_data.coordinate_data[COORD_C];

                    // and save to parameters
                    Settings_Manager::WriteCoordinateValues(92, &values[0]);
                }
                break;  

                case 1:     // G92.1 [Reset coordinate offsets and save to parameters]
                case 2:     // G92.2 [Reset coordinate offsets but not save]
                {
                    memset((void*)&values[0], 0, sizeof(values));

                    // Only save if G92.1
                    if (action == 1)
                        Settings_Manager::WriteCoordinateValues(92, &values[0]);
                }
                break;

                default:    // G92.3 [Recall coordinate offsets from parameters]
                {
                    Settings_Manager::ReadCoordinateValues(92, &values[0]);
                }
                break;
            }
            
            // In any case update active coordinate offsets
            memcpy((void*)&m_g92_coord_offset[0], (const void*)&values[0], sizeof(m_g92_coord_offset));
        }
        break;
    }

    return work_var;
}

int GCodeParser::handle_motion_commands()
{
    bool cw_arc = false;
    float target[TOTAL_AXES_COUNT];
    uint32_t index;
    int32_t work_var;

    m_parser_modal_state.motion_mode = m_block_data.block_modal_state.motion_mode;
    
    // Update target position depending on current status [keep unspecified values]
    memcpy(&target[0], &m_gcode_machine_pos[0], sizeof(target));
    
    // Check if using Absolute Override (G53)
    if (m_block_data.non_modal_code != NON_MODAL_ABSOLUTE_OVERRIDE)
    {
        // Not using G53
        if (m_parser_modal_state.distance_mode == MODAL_DISTANCE_MODE_ABSOLUTE)
        {
            // Take into account WCS, G92 and tool offsets
            for (index = COORD_X; index < TOTAL_AXES_COUNT; index++)
            {
                if (((1 << index) & m_value_group_flags) != 0)
                {
                    // Current coordinate was specified. Check if linear or rotational
                    if (index < COORD_A)
                        target[index] = convert_to_mm(m_block_data.coordinate_data[index]);
                    else
                        target[index] = m_block_data.coordinate_data[index];
                    
                    // Apply offsets
                    target[index] += (m_work_coord_sys[index] - m_g92_coord_offset[index]);
                    
                    // Only apply tool offsets for linear axes
                    if (index < COORD_A)
                        target[index] += m_tool_offset[index];
                }
            }
        }
        else
        {
            // The values specified are increments
            for (index = COORD_X; index < TOTAL_AXES_COUNT; index++)
            {
                if (((1 << index) & m_value_group_flags) != 0)
                {
                    // Current coordinate was specified. Check if linear or rotational
                    if (index < COORD_A)
                        target[index] += convert_to_mm(m_block_data.coordinate_data[index]);
                    else
                        target[index] += m_block_data.coordinate_data[index];
                }
            }
        }
    }
    else
    {
        // G53 active. Coordinate data is already in absolute machine coordinates
        for (index = COORD_X; index < TOTAL_AXES_COUNT; index++)
        {
            if (((1 << index) & m_value_group_flags) != 0)
            {
                // Current coordinate was specified. Check if linear or rotational
                if (index < COORD_A)
                    target[index] = convert_to_mm(m_block_data.coordinate_data[index]);
                else
                    target[index] = m_block_data.coordinate_data[index];
            }
        }
    }
    
    switch (m_parser_modal_state.motion_mode)
    {
        case MODAL_MOTION_MODE_SEEK:
        case MODAL_MOTION_MODE_LINEAR_FEED:
        {
			// In this mode we should have all the information required to perform the Seek move [X,Y,Z,A,B,C]
			work_var = this->motion_append_line(&target[0]);
            
            if (work_var != GCODE_OK)
                return work_var;
            
            // Update global machine position after performing move
            memcpy(&m_gcode_machine_pos[0], &target[0], sizeof(m_gcode_machine_pos));
        }
        break;

        case MODAL_MOTION_MODE_HELICAL_CW:
			cw_arc = true;
        case MODAL_MOTION_MODE_HELICAL_CCW:
		{
            float offsets[3];
            float radius;
            float delta_axis_0, delta_axis_1;
            
            // Calculate distance between current position and target position
            delta_axis_0 = target[m_axis_zero] - m_gcode_machine_pos[m_axis_zero];
            delta_axis_1 = target[m_axis_one]  - m_gcode_machine_pos[m_axis_one];

            // This code is a mix between GRBL and Smoothieware arc handling.
            // This first part is based on GRBL to allow the specification of R-value arcs
            // and also I,J,K offset arcs. 
            
            // Determine the format specification of the arc [Radius/Center based]
            if ((m_value_group_flags & VALUE_SET_R_BIT) != 0)
            {
                float h_x2_div_d;
                                
                // Check if endpoint equals to current point in Radius format arcs
                if (memcmp((const void*)&target[0], (const void*)&m_gcode_machine_pos[0], sizeof(target)) == 0)
                    return GCODE_ERROR_INVALID_TARGET_FOR_R_ARC;
                
                // Radius format arc. R value provided. Need to calculate I, J, K offsets
                radius = convert_to_mm(m_block_data.R_value);
                
                h_x2_div_d = 4.0f * radius * radius - (delta_axis_0 * delta_axis_0) - (delta_axis_1 * delta_axis_1);

                // Check the h_x2_div_d value prior to applying sqrt. Negative values indicate an error
                if (h_x2_div_d < 0.0f) 
                    return GCODE_ERROR_INVALID_R_VALUE;
                
                // Finish the calculation of h_x2_div_d
                h_x2_div_d = -sqrtf(h_x2_div_d) / hypotf(delta_axis_0, delta_axis_1);
                
                // If arc is CCW then invert the sign of h_x2_div_d
                if (cw_arc == false)
                    h_x2_div_d = -h_x2_div_d;
                
                // Check if radius < 0 (indicates an arc of > 180 degrees
                if (radius < 0.0f)
                {
                    h_x2_div_d = -h_x2_div_d;
                    radius = -radius;
                }
                
                // Finally calculate the offsets
                offsets[m_axis_zero] = 0.5f * (delta_axis_0 - (delta_axis_1 * h_x2_div_d));
                offsets[m_axis_one]  = 0.5f * (delta_axis_1 + (delta_axis_0 * h_x2_div_d));
                offsets[m_axis_linear] = 0.0f;
            }
            else
            {
                float delta_r_checking;
                float target_radius;
                
                // Center format arc. I, J, K values provided. Need to calculate R value. 
                for (index = 0; index < 3; index++)
                {
                    if ((m_value_group_flags & (1 << (index + 6))) != 0)
                        offsets[index] = convert_to_mm(m_block_data.offset_ijk_data[index]);
                    else
                        offsets[index] = 0.0f;
                }
                
                delta_axis_0 = delta_axis_0 - offsets[m_axis_zero];
                delta_axis_1 = delta_axis_1 - offsets[m_axis_one];
                
                target_radius = hypotf(delta_axis_0, delta_axis_1);
                
                // Calculate radius based only on offsets
                radius = hypotf(offsets[m_axis_zero], offsets[m_axis_one]);
                
                // Check if distance from center to current point differs from 
                // distance from center to endpoint for an specific tolerance (0.1%)
                delta_r_checking = fabsf(target_radius - radius);
                
                if (delta_r_checking > 0.005f)
                {
                    if (delta_r_checking > 0.5f)
                        return GCODE_ERROR_INVALID_TARGET_FOR_OFS_ARC;
                    
//                    if (delta_r_checking > (0.001f * radius))
//                        return GCODE_ERROR_INVALID_TARGET_FOR_ARC;
                }
            }

			// In this point we should have all the required information to perform an arc move [X,Y,Z,A,B,C, F, R | (I,J,K)]
			// The following processing is based on Smoothieware
            float center_axis0 = m_gcode_machine_pos[m_axis_zero] + offsets[m_axis_zero];
            float center_axis1 = m_gcode_machine_pos[m_axis_one]  + offsets[m_axis_one];
            float linear_travel = target[m_axis_linear] - m_gcode_machine_pos[m_axis_linear];

            float r_axis0 = -offsets[m_axis_zero]; // Radius vector from center to start position
            float r_axis1 = -offsets[m_axis_one];

            float rt_axis0 = target[m_axis_zero] - m_gcode_machine_pos[m_axis_zero] - offsets[m_axis_zero]; // Radius vector from center to target position
            float rt_axis1 = target[m_axis_one]  - m_gcode_machine_pos[m_axis_one]  - offsets[m_axis_one];
            float angular_travel = 0.0f;
            
            //check for condition where atan2 formula will fail due to everything canceling out exactly
            if ((m_gcode_machine_pos[m_axis_zero] == target[m_axis_zero]) && 
                (m_gcode_machine_pos[m_axis_one]  == target[m_axis_one])) 
            {
                if (cw_arc == true)
                    angular_travel = (-2 * M_PI);   // set angular_travel to -2pi for a CW full circle
                else 
                    angular_travel = (+2 * M_PI);   // set angular_travel to +2pi for a CCW full circle
            }
            else 
            {
                // Patch from GRBL Firmware - Christoph Baumann 04072015
                // CCW angle between position and target from circle center. Only one atan2() trig computation required.
                // Only run if not a full circle or angular travel will incorrectly result in 0.0f
                angular_travel = atan2f(r_axis0 * rt_axis1 - r_axis1 * rt_axis0, 
                                        r_axis0 * rt_axis0 + r_axis1 * rt_axis1);
                
                if (m_axis_linear == COORD_Y) 
                    cw_arc = !cw_arc; //Math for XZ plane is reverse of other 2 planes
                
                if (cw_arc == true)
                { 
                    // adjust angular_travel to be in the range of -2pi to 0 for clockwise arcs
                    if (angular_travel > 0.0f)
                        angular_travel -= (2 * M_PI);
                }
                else 
                {
                    // adjust angular_travel to be in the range of 0 to 2pi for counterclockwise arcs
                    if (angular_travel < 0.0f) 
                        angular_travel += (2 * M_PI); 
                }
            }

            // Find the distance for this gcode
            float millimeters_of_travel = hypotf(angular_travel * radius, fabsf(linear_travel));

            // We don't care about non-XYZ moves
            if ( millimeters_of_travel < 0.000001f )
                return GCODE_OK;

            // limit segments by maximum arc error
            float arc_segment = this->m_mm_per_arc_segment;
            
            if ((this->m_mm_max_arc_error > 0) && (2.0f * radius > this->m_mm_max_arc_error)) 
            {
                float min_err_segment = 2.0f * sqrtf((this->m_mm_max_arc_error * (2.0f * radius - this->m_mm_max_arc_error)));
        
                if (this->m_mm_per_arc_segment < min_err_segment) 
                {
                    arc_segment = min_err_segment;
                }
            }
            
            // catch fall through on above
            if (arc_segment < 0.0001f) 
                arc_segment = 0.5F; /// the old default, so we avoid the divide by zero

            // Figure out how many segments for this gcode
            uint16_t segments = floorf(millimeters_of_travel / arc_segment);

            if (segments > 1) 
            {
                float theta_per_segment = angular_travel / segments;
                float linear_per_segment = linear_travel / segments;

                /* Vector rotation by transformation matrix: r is the original vector, r_T is the rotated vector,
                and phi is the angle of rotation. Based on the solution approach by Jens Geisler.
                r_T = [cos(phi) -sin(phi);
                sin(phi) cos(phi] * r ;
                For arc generation, the center of the circle is the axis of rotation and the radius vector is
                defined from the circle center to the initial position. Each line segment is formed by successive
                vector rotations. This requires only two cos() and sin() computations to form the rotation
                matrix for the duration of the entire arc. Error may accumulate from numerical round-off, since
                all float numbers are single precision on the Arduino. (True float precision will not have
                round off issues for CNC applications.) Single precision error can accumulate to be greater than
                tool precision in some cases. Therefore, arc path correction is implemented.

                Small angle approximation may be used to reduce computation overhead further. This approximation
                holds for everything, but very small circles and large mm_per_arc_segment values. In other words,
                theta_per_segment would need to be greater than 0.1 rad and N_ARC_CORRECTION would need to be large
                to cause an appreciable drift error. N_ARC_CORRECTION~=25 is more than small enough to correct for
                numerical drift error. N_ARC_CORRECTION may be on the order a hundred(s) before error becomes an
                issue for CNC machines with the single precision Arduino calculations.
                This approximation also allows mc_arc to immediately insert a line segment into the planner
                without the initial overhead of computing cos() or sin(). By the time the arc needs to be applied
                a correction, the planner should have caught up to the lag caused by the initial mc_arc overhead.
                This is important when there are successive arc motions.
                */
                // Vector rotation matrix values
                float cos_T = 1.0f - (0.5f * theta_per_segment * theta_per_segment); // Small angle approximation
                float sin_T = theta_per_segment;

                // TODO we need to handle the ABC axis here by segmenting them
                float arc_target[TOTAL_AXES_COUNT];
                float sin_Ti;   
                float cos_Ti;
                float r_axisi;
                int8_t count = 0;

                // init array for all axis
                memcpy((void*)&arc_target[0], &m_gcode_machine_pos[0], sizeof(arc_target));

                // Initialize the linear axis (redundant?)
                arc_target[m_axis_linear] = m_gcode_machine_pos[m_axis_linear];

                for (index = 1; index < segments; index++) 
                {
                    // Increment (segments-1)
                    // Check for abort conditions
                    if (machine->IsHalted() == true)
                        break;
                    
                    if (count < m_arc_correction_counter) 
                    {
                        // Apply vector rotation matrix
                        r_axisi = r_axis0 * sin_T + r_axis1 * cos_T; // temp = r_axis0 * sin(theta) + r_axis1 * cos(theta)
                        r_axis0 = r_axis0 * cos_T - r_axis1 * sin_T; // r_axis0' = r_axis0 * cos(theta) - r_axis1 * sin(theta)
                        r_axis1 = r_axisi;                           // r_axis1' = temp
                        count++;
                    }
                    else 
                    {
                        // Arc correction to radius vector. Computed only every N_ARC_CORRECTION increments.
                        // Compute exact location by applying transformation matrix from initial radius vector(=-offset).
                        cos_Ti = cosf(index * theta_per_segment);
                        sin_Ti = sinf(index * theta_per_segment);
                        
                        r_axis0 = -offsets[m_axis_zero] * cos_Ti + offsets[m_axis_one] * sin_Ti;
                        r_axis1 = -offsets[m_axis_zero] * sin_Ti - offsets[m_axis_one] * cos_Ti;
                        count = 0;
                    }

                    // Update arc_target location
                    arc_target[m_axis_zero] = center_axis0 + r_axis0;
                    arc_target[m_axis_one] = center_axis1 + r_axis1;
                    arc_target[m_axis_linear] += linear_per_segment;

                    // Append this segment to the queue
                    work_var = this->motion_append_line(arc_target);
                    
                    if (work_var != GCODE_OK)
                        return work_var;
                }
            }
            
            // Ensure to add at least one move
            work_var = this->motion_append_line(target);
            
            if (work_var != GCODE_OK)
                return work_var;
            
            // Update global machine position after performing move
            memcpy(&m_gcode_machine_pos[0], &target[0], sizeof(m_gcode_machine_pos));
		}
        break;

        case MODAL_MOTION_MODE_PROBE:   // TODO: Implement probing
		{
			// Check if current feedrate mode is units/min. Flag error otherwise
			if (m_parser_modal_state.feedrate_mode != MODAL_FEEDRATE_MODE_UNITS_PER_MIN)
				return GCODE_ERROR_PROBE_CANNOT_USE_INVERSE_TIME_FEEDRATE_MODE;

			// TODO: Check the distance to move is < 0.254mm

			// Check if any of the rotary axes moves
			if ((m_block_data.coordinate_data[COORD_A] != m_gcode_machine_pos[COORD_A]) ||
				(m_block_data.coordinate_data[COORD_B] != m_gcode_machine_pos[COORD_B]) ||
				(m_block_data.coordinate_data[COORD_C] != m_gcode_machine_pos[COORD_C]))
			{
				// Rotary axes cannot move during probe cycle
				return GCODE_ERROR_PROBE_CANNOT_MOVE_ROTATY_AXES;
			}			
		}
        break;

        default:
        {
            if (m_canned_cycle_active != false)
            {
                if (m_parser_modal_state.distance_mode == MODAL_DISTANCE_MODE_ABSOLUTE)
                {
                    // Update sticky values
                    canned_cycle_update_sticky();
                    
                    switch (m_parser_modal_state.motion_mode)
                    {
                        case MODAL_MOTION_MODE_CANNED_DRILL_G81:
                        {
                            // G0 X... Y...
                            // G0 Z[=R]
                            
                            // G1 Z[=sticky_z] F[=sticky_f]
                            
                            // G0 Z[=r_plane]
                        }
                        break;
                        
                        case MODAL_MOTION_MODE_CANNED_DRILL_DWELL_G82:
                        {
                            // G0 X... Y...
                            // G0 Z[=R]
                            
                            // G1 Z[=sticky_z] F[=sticky_f]
                            // G4 P...
                            
                            // G0 Z[=r_plane]
                        }
                        break;
                        
                        case MODAL_MOTION_MODE_CANNED_DRILL_PECK_G83:
                        {
                            // G0 X... Y...
                            // G0 Z[=R]
                            
                            // @depth = sticky_r - sticky_z
                            // @cycles = @depth / sticky_q
                            // @rest = @depth MOD sticky_q
                            
                            // ... TODO: 
                            
                            
                            // G0 Z[=r_plane]
                        }
                        break;
                        
//                        case MODAL_MOTION_MODE_CANNED_TAPPING_G84:
//                        case MODAL_MOTION_MODE_CANNED_BORING_G85:
//                        case MODAL_MOTION_MODE_CANNED_BORING_DWELL_G86:
//                        case MODAL_MOTION_MODE_CANNED_COUNTERBORING_G87:
//                        case MODAL_MOTION_MODE_CANNED_BORING_G88:
//                        case MODAL_MOTION_MODE_CANNED_BORING_G89:
                        
                        default:
                            break;
                    }
                }
            }
        }
        break;
    }
    
    return GCODE_OK;
}

// TODO: Check this code for redundancy
int GCodeParser::motion_append_line(const float * target_pos)
{
    uint32_t index;
    
    // check soft limits only for homed axis that are enabled
    if (Settings_Manager::AreSoftLimitsEnabled() == true)
    {
        for (index = COORD_X; index < COORDINATE_LINEAR_AXES_COUNT; index++)
        {
            // Skip if we're homing (any axis)/not homed yet (this axis)
            if (machine->IsHomingNow() == true || machine->IsAxisHomed(index) == false)
                continue;

            if (index < COORD_A)
            {
                if (this->m_soft_limit_enabled[index] != false)
                {
                    if ((target_pos[index] < 0.0f) ||
                        (target_pos[index] > this->m_soft_limit_max_values[index]))
                    {
                        // Report violation of limit values. Stop the execution of commands.
                        machine->Halt();
                        return GCODE_ERROR_TARGET_OUTSIDE_LIMIT_VALUES;
                    }
                }
            }
        }
    }
    
    // If currently in check mode then stop processing here.
    if (m_check_mode != false)
        return GCODE_OK;
    
    // Check if currently in feed hold condition
    while (machine->IsFeedHoldActive() == true)
    {
        // Wait here.
        // TODO: Check what else to do during this time. The idea is to use this class
        //       from inside a task, so the task can be put in a wait state until feed hold
        //       condition disappears. 
        vTaskDelay(pdMS_TO_TICKS(100)); // Yield control to other tasks while the queue is being flushed
    }

    // Finally call the planner to append a new block.
    if (m_planner_ref != NULL)
    {
        float move_rate = (m_parser_modal_state.motion_mode == MODAL_MOTION_MODE_SEEK) ? SOME_LARGE_VALUE : (m_block_data.feed_rate / 60.0f);
        bool inverse_time_rate = (m_parser_modal_state.feedrate_mode == MODAL_FEEDRATE_MODE_INVERSE_TIME) ? true : false;
        
        // Inverse time feed rate mode does not affect Seek Movements
        if (m_parser_modal_state.motion_mode == MODAL_MOTION_MODE_SEEK)
            inverse_time_rate = false;
        
        return m_planner_ref->AppendLine(target_pos, m_spindle_speed, move_rate, inverse_time_rate);
    }
    
    return GCODE_ERROR_MISSING_PLANNER;    
}

float GCodeParser::convert_to_mm(float value)
{
    if (m_parser_modal_state.units_mode == MODAL_UNITS_MODE_INCHES)
        return (value * MM_PER_INCH);
    
    return value;
}


void GCodeParser::canned_cycle_reset_stycky()
{    
    m_cc_sticky_z = 0.0f;
    m_cc_sticky_r = 0.0f;
    m_cc_sticky_f = 0.0f;
    m_cc_sticky_q = 0.0f;
    m_cc_sticky_p = 0.0f;
}

void GCodeParser::canned_cycle_update_sticky()
{
    if ((m_value_group_flags & VALUE_SET_Z_BIT) != 0)
        m_cc_sticky_z = m_block_data.coordinate_data[COORD_Z];
    
    if ((m_value_group_flags & VALUE_SET_R_BIT) != 0)
        m_cc_sticky_r = m_block_data.R_value;
    
    if ((m_value_group_flags & VALUE_SET_F_BIT) != 0)
        m_cc_sticky_f = m_block_data.feed_rate;
    
    if ((m_value_group_flags & VALUE_SET_Q_BIT) != 0)
        m_cc_sticky_q = m_block_data.Q_value;
    
    if ((m_value_group_flags & VALUE_SET_P_BIT) != 0)
        m_cc_sticky_p = m_block_data.P_value;
    
    // Update return position value
    if (m_parser_modal_state.canned_return_mode == MODAL_CANNED_RETURN_TO_PREV_POSITION)
        m_cc_r_plane = m_cc_initial_z;
    else
        m_cc_r_plane = m_cc_sticky_r;
}

const char* GCodeParser::GetErrorText(uint32_t error_code)
{
    switch (error_code)
    {
    case GCODE_OK:  
        return("OK");        

    case GCODE_INFO_BLOCK_DELETE:
        return("Block deleted");
        
    case GCODE_ERROR_MISSING_PLANNER:
        return("Missing motion controller");
        
    case GCODE_ERROR_INVALID_LINE_NUMBER:
        return("Invalid line number");
        
    case GCODE_ERROR_NESTED_COMMENT:
        return("Nested comment");
        
    case GCODE_ERROR_UNOPENED_COMMENT:
        return("Unopened comment");
                
    case GCODE_ERROR_UNCLOSED_COMMENT:
        return("Unclosed comment");

    case GCODE_ERROR_UNSUPPORTED_CODE_FOUND:
        return("Unsupported code found");
        
    case GCODE_ERROR_MULTIPLE_DEF_SAME_COORD:
        return("Multiple definitions of same coordinate");
        
    case GCODE_ERROR_INVALID_COORDINATE_VALUE:
        return("Invalid coordinate value");
        
    case GCODE_ERROR_MULTIPLE_DEF_SAME_OFFSET:
        return("Multiple definitions of same offset");
        
    case GCODE_ERROR_INVALID_OFFSET_VALUE:
        return("Invalid offset value");
        
    case GCODE_ERROR_MULTIPLE_DEF_FEEDRATE:
        return("Multiple definitions of feed rate");
        
    case GCODE_ERROR_INVALID_FEEDRATE_VALUE:
        return("Invalid feed rate value");
        
    case GCODE_ERROR_MULTIPLE_DEF_L_WORD:
        return("Multiple definitions of L word");
        
    case GCODE_ERROR_INVALID_L_VALUE:
        return("Invalid L value");
        
    case GCODE_ERROR_MULTIPLE_DEF_P_WORD:
        return("Multiple definitions of P word");
                
    case GCODE_ERROR_INVALID_P_VALUE:
        return("Invalid P value");
        
    case GCODE_ERROR_MULTIPLE_DEF_R_WORD:
        return("Multiple definitions of R word");
        
    case GCODE_ERROR_INVALID_R_VALUE:
        return("Invalid R value");
        
    case GCODE_ERROR_MULTIPLE_DEF_SPINDLE_SPEED:
        return("Multiple definitions of spindle speed");
        
    case GCODE_ERROR_INVALID_SPINDLE_SPEED:
        return("Invalid spindle speed");
        
    case GCODE_ERROR_MULTIPLE_DEF_TOOL_INDEX:
        return("Multiple definitions of tool index");
        
    case GCODE_ERROR_INVALID_TOOL_INDEX:
        return("Invalid tool index");
        
    case GCODE_ERROR_MODAL_M_GROUP_VIOLATION:
        return("Modal M group code violation");
        
    case GCODE_ERROR_INVALID_M_CODE:
        return("Invalid M code found");
        
    case GCODE_ERROR_MODAL_G_GROUP_VIOLATION:
        return("Modal G group code violation");
        
    case GCODE_ERROR_MODAL_AXIS_CONFLICT:
        return("Modal axis conflict found");
        
    case GCODE_ERROR_INVALID_G_CODE:
        return("Unsupported G code found");
            
    case GCODE_ERROR_MISSING_COORDS_IN_MOTION:
        return("Missing all coordinate values in motion");
        
    case GCODE_ERROR_AMBIGUOUS_ARC_MODE:
        return("Ambiguous arc mode");
        
    case GCODE_ERROR_UNSPECIFIED_ARC_MODE:
        return("Unspecified arc mode");
        
    case GCODE_ERROR_ENDPOINT_ARC_INVALID:
        return("Invalid arc endpoint");
            
    case GCODE_ERROR_MISSING_DWELL_TIME:
        return("Missing dwell time");
        
    case GCODE_ERROR_MISSING_XYZ_G10_CODE:
        return("Missing coordinates for G10 code");
        
    case GCODE_ERROR_MISSING_L_P_G10_CODE:
        return("Missing L/P values for G10 code");
            
    case GCODE_ERROR_MISSING_XYZ_G92_CODE:
        return("Missing coordinates for G92 code");
        
    case GCODE_ERROR_INVALID_G53_MOTION_MODE:
        return("Invalid G53 motion code found");
        
    case GCODE_ERROR_COORD_DATA_IN_G80_CODE:
        return("Unexpected coordinate data in G80 code");
            
    case GCODE_ERROR_MISSING_COORDS_IN_PLANE:
        return("Missing required coordinates in active plane");
        
    case GCODE_ERROR_INVALID_TARGET_FOR_R_ARC:
        return("Invalid target for radius format arc");
    
    case GCODE_ERROR_INVALID_TARGET_FOR_OFS_ARC:
        return("Invalid target for center format arc");
        
    case GCODE_ERROR_INVALID_TARGET_FOR_PROBE:
        return("Invalid target for probe");
       
    case GCODE_ERROR_UNUSED_OFFSET_VALUE_WORDS:
        return("Found unused [I,J,K] offset value words");
    
    case GCODE_ERROR_UNUSED_L_VALUE_WORD:
        return("Found unused L value word");
    
    case GCODE_ERROR_UNUSED_P_VALUE_WORD:
        return("Found unused P value word");
    
    case GCODE_ERROR_UNUSED_Q_VALUE_WORD:
        return("Found unused Q value word");
    
    case GCODE_ERROR_UNUSED_R_VALUE_WORD:
        return("Found unused R value word");
       
    case GCODE_ERROR_USING_COORDS_NO_MOTION_MODE:
        return("Unexpected coordinates in no motion mode");
        
    case GCODE_ERROR_INVALID_G53_DISTANCE_MODE:
        return("Invalid G53 distance mode");
        
    case GCODE_ERROR_INVALID_NON_MODAL_COMMAND:
        return("Unsupported non modal command found");

    case GCODE_ERROR_USING_D_WORD_OUTSIDE_G41_G42:
        return("Unexpected D word outside G41/G42 codes");
        
    case GCODE_ERROR_USING_H_WORD_OUTSIDE_G43:
        return("Unexpected H word outside G43 code");
        
    case GCODE_ERROR_MULTIPLE_DEF_Q_WORD:
        return("Multiple definitions of Q word");
        
    case GCODE_ERROR_INVALID_Q_VALUE:
        return("Invalid Q word value");
    
    case GCODE_ERROR_MULTIPLE_DEF_D_WORD:
        return("Multiple definitions of D word");
    
    case GCODE_ERROR_INVALID_D_VALUE:
        return("Invalid D word value");
    
    case GCODE_ERROR_MULTIPLE_DEF_H_WORD:
        return("Multiple definitions of H word");
        
    case GCODE_ERROR_INVALID_H_VALUE:
        return("Invalid H word value");
    
    case GCODE_ERROR_MISSING_ARC_DATA_HELICAL_MOTION:
        return("Missing arc data [Radius value/IJK Offsets] in helical [G2/G3] motion");

    case GCODE_ERROR_CONFLICT_ARC_DATA_HELICAL_MOTION:
        return("Conflict in arc data [Radius value/IJK Offsets] in helical [G2/G3] motion");

	case GCODE_ERROR_MISSING_OFFSETS_IN_PLANE:
		return("Missing coordinate offsets [IJK] for arc in the active plane");

	case GCODE_ERROR_PROBE_CANNOT_USE_INVERSE_TIME_FEEDRATE_MODE:
		return("Cannot perform probe cycle with Inverse time feed rate mode");

	case GCODE_ERROR_PROBE_CANNOT_MOVE_ROTATY_AXES:
		return("Cannot perform probe cycle with rotary axes movement");
    
	case GCODE_ERROR_ABSOLUTE_OVERRIDE_CANNOT_USE_RAD_COMP:
		return("G53 cannot be used while cutter radius compensation is active");

    case GCODE_ERROR_CANNOT_CHANGE_WCS_WITH_RAD_COMP:
        return("Cannot change WCS while tool radius compensation is active");
    
	case GCODE_ERROR_MISSING_FEEDRATE_INVERSE_TIME_MODE:
		return("Missing feed rate value with inverse time feed rate mode moves");
        
    case GCODE_ERROR_TARGET_OUTSIDE_LIMIT_VALUES:
        return("The specified target is outside of the limit values");
        
    case GCODE_ERROR_HOMING_LOCATE_LIMITS:
        return("Homing Error: Could not locate all limit switches");
        
    case GCODE_ERROR_HOMING_RELEASE_LIMITS:
        return("Homing Error: Limit switch(es) still engaged");
        
    case GCODE_ERROR_HOMING_CONFIRM_LIMITS:
        return("Homing Error: Could not confirm all limit switches");
        
    case GCODE_ERROR_HOMING_FINISH_RELEASE:
        return("Homing Error: Could not complete homing. Limit switch(es) still engaged");
    
    default:
        return("Unknown error code");
    }
}

void GCodeParser::ReadParserModalState(GCodeModalData* block) const
{
    memcpy(block, &this->m_parser_modal_state, sizeof(GCodeModalData));
}
