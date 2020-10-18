#ifndef GCODEPARSER_H
#define GCODEPARSER_H

#include <stdint.h>
#include "DataConverter.h"


///////////////////////////////////////////////////////////////////////////////

#define MM_PER_INCH     25.4f
#define M_PI            3.1415926535897932384626433832795f

#define COORDINATE_LINEAR_AXES_COUNT		3
#define COORDINATE_ROTATIONAL_AXES_COUNT	3
#define TOTAL_AXES_COUNT    (COORDINATE_LINEAR_AXES_COUNT + COORDINATE_ROTATIONAL_AXES_COUNT)

#define TOTAL_WCS_COUNT                     9

#define COORD_X     0
#define COORD_Y	    1
#define COORD_Z     2
#define COORD_A	    3
#define COORD_B	    4
#define COORD_C	    5


#define SOME_LARGE_VALUE       (1.0e+38f)

///////////////////////////////////////////////////////////////////////////////

#define INCLUDE_AXIS_A      1
#define INCLUDE_AXIS_B      1
#define INCLUDE_AXIS_C      1

///////////////////////////////////////////////////////////////////////////////

enum GCODE_STATUS_RESULTS 
{
	GCODE_OK = 0,
	GCODE_INFO_BLOCK_DELETE, 

	GCODE_ERROR_MISSING_PLANNER,

	GCODE_ERROR_INVALID_LINE_NUMBER,        // 3
	GCODE_ERROR_NESTED_COMMENT,
	GCODE_ERROR_UNOPENED_COMMENT,
	GCODE_ERROR_UNCLOSED_COMMENT,
	GCODE_ERROR_UNSUPPORTED_CODE_FOUND,

	GCODE_ERROR_MULTIPLE_DEF_SAME_COORD,    // 7
	GCODE_ERROR_INVALID_COORDINATE_VALUE,

	GCODE_ERROR_MULTIPLE_DEF_SAME_OFFSET,   // 9
	GCODE_ERROR_INVALID_OFFSET_VALUE,

	GCODE_ERROR_MULTIPLE_DEF_FEEDRATE,      // 11
	GCODE_ERROR_INVALID_FEEDRATE_VALUE,

	GCODE_ERROR_MULTIPLE_DEF_L_WORD,        // 13
	GCODE_ERROR_INVALID_L_VALUE,

	GCODE_ERROR_MULTIPLE_DEF_P_WORD,        // 15
	GCODE_ERROR_INVALID_P_VALUE, 

	GCODE_ERROR_MULTIPLE_DEF_R_WORD,        // 17
	GCODE_ERROR_INVALID_R_VALUE,

	GCODE_ERROR_MULTIPLE_DEF_SPINDLE_SPEED, // 19
	GCODE_ERROR_INVALID_SPINDLE_SPEED,

	GCODE_ERROR_MULTIPLE_DEF_TOOL_INDEX,    // 21
	GCODE_ERROR_INVALID_TOOL_INDEX,

	GCODE_ERROR_MODAL_M_GROUP_VIOLATION,    // 23
	GCODE_ERROR_INVALID_M_CODE,

	GCODE_ERROR_MODAL_G_GROUP_VIOLATION,    // 25
    GCODE_ERROR_MODAL_AXIS_CONFLICT,
	GCODE_ERROR_INVALID_G_CODE,
    
    GCODE_ERROR_MISSING_COORDS_IN_MOTION,   // 28
    GCODE_ERROR_AMBIGUOUS_ARC_MODE,
    GCODE_ERROR_UNSPECIFIED_ARC_MODE,       
    GCODE_ERROR_ENDPOINT_ARC_INVALID,           
    
    GCODE_ERROR_MISSING_DWELL_TIME,         // 32
    GCODE_ERROR_MISSING_XYZ_G10_CODE,       
    GCODE_ERROR_MISSING_L_P_G10_CODE,
    
    GCODE_ERROR_MISSING_XYZ_G92_CODE,       // 35
    GCODE_ERROR_INVALID_G53_MOTION_MODE,
    GCODE_ERROR_COORD_DATA_IN_G80_CODE, 
    
    GCODE_ERROR_MISSING_COORDS_IN_PLANE,    // 38
    GCODE_ERROR_INVALID_TARGET_FOR_R_ARC,
    GCODE_ERROR_INVALID_TARGET_FOR_OFS_ARC,
    GCODE_ERROR_INVALID_TARGET_FOR_PROBE,
    GCODE_ERROR_UNUSED_OFFSET_VALUE_WORDS,
    GCODE_ERROR_UNUSED_L_VALUE_WORD,
    GCODE_ERROR_UNUSED_P_VALUE_WORD,
    GCODE_ERROR_UNUSED_Q_VALUE_WORD,
    GCODE_ERROR_UNUSED_R_VALUE_WORD,

    GCODE_ERROR_USING_COORDS_NO_MOTION_MODE, // 42
    GCODE_ERROR_INVALID_G53_DISTANCE_MODE,
    GCODE_ERROR_INVALID_NON_MODAL_COMMAND,

    GCODE_ERROR_USING_D_WORD_OUTSIDE_G41_G42, // 45
    GCODE_ERROR_USING_H_WORD_OUTSIDE_G43,

    GCODE_ERROR_MULTIPLE_DEF_Q_WORD,
    GCODE_ERROR_INVALID_Q_VALUE,
    
    GCODE_ERROR_MULTIPLE_DEF_D_WORD,
    GCODE_ERROR_INVALID_D_VALUE,
    
    GCODE_ERROR_MULTIPLE_DEF_H_WORD,
    GCODE_ERROR_INVALID_H_VALUE,

    GCODE_ERROR_MISSING_ARC_DATA_HELICAL_MOTION,  // 
    GCODE_ERROR_CONFLICT_ARC_DATA_HELICAL_MOTION,
	GCODE_ERROR_MISSING_OFFSETS_IN_PLANE,

	GCODE_ERROR_PROBE_CANNOT_USE_INVERSE_TIME_FEEDRATE_MODE,
	GCODE_ERROR_PROBE_CANNOT_MOVE_ROTATY_AXES,

	GCODE_ERROR_ABSOLUTE_OVERRIDE_CANNOT_USE_RAD_COMP,
	GCODE_ERROR_CANNOT_CHANGE_WCS_WITH_RAD_COMP,
	GCODE_ERROR_MISSING_FEEDRATE_INVERSE_TIME_MODE,
    
    GCODE_ERROR_TARGET_OUTSIDE_LIMIT_VALUES,
    
    /* Homing related errors */
    GCODE_ERROR_HOMING_LOCATE_LIMITS,
    GCODE_ERROR_HOMING_RELEASE_LIMITS,
    GCODE_ERROR_HOMING_CONFIRM_LIMITS,
    GCODE_ERROR_HOMING_FINISH_RELEASE,
    
    
};

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_MOTION_MODES
{
    MODAL_MOTION_MODE_SEEK = 0,             // G0
    MODAL_MOTION_MODE_LINEAR_FEED = 10,     // G1
    MODAL_MOTION_MODE_HELICAL_CW = 20,      // G2
    MODAL_MOTION_MODE_HELICAL_CCW = 30,     // G3        
    MODAL_MOTION_MODE_PROBE = 382,          // G38.2
    MODAL_MOTION_MODE_CANCEL_MOTION = 800,  // G80

    MODAL_MOTION_MODE_CANNED_DRILL_G81 = 810,   // G81
    MODAL_MOTION_MODE_CANNED_DRILL_DWELL_G82 = 820, // G82
    MODAL_MOTION_MODE_CANNED_DRILL_PECK_G83 = 830, // G83
//    MODAL_MOTION_MODE_CANNED_TAPPING_G84 = 840, // G84
//    MODAL_MOTION_MODE_CANNED_BORING_G85 = 850, // G85
//    MODAL_MOTION_MODE_CANNED_BORING_DWELL_G86 = 860, // G86
//    MODAL_MOTION_MODE_CANNED_COUNTERBORING_G87 = 870, // G87
//    MODAL_MOTION_MODE_CANNED_BORING_G88 = 880, // G88
//    MODAL_MOTION_MODE_CANNED_BORING_G89 = 890  // G89

}GCODE_MODAL_MOTION_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_FEEDRATE_MODES
{
    MODAL_FEEDRATE_MODE_UNITS_PER_MIN = 940,    // G94
    MODAL_FEEDRATE_MODE_INVERSE_TIME = 930      // G93
}GCODE_MODAL_FEEDRATE_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_UNITS_MODES
{
    MODAL_UNITS_MODE_INCHES = 200,              // G20
    MODAL_UNITS_MODE_MILLIMETERS = 210          // G21
}GCODE_MODAL_UNITS_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_DISTANCE_MODES
{
    MODAL_DISTANCE_MODE_ABSOLUTE = 900,         // G90
    MODAL_DISTANCE_MODE_INCREMENTAL = 910       // G91
}GCODE_MODAL_DISTANCE_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_PLANE_SELECT_MODES
{
    MODAL_PLANE_SELECT_XY = 170,                // G17    
    MODAL_PLANE_SELECT_XZ = 180,                // G18
    MODAL_PLANE_SELECT_YZ = 190                 // G19
}GCODE_MODAL_PLANE_SELECT_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_PROG_FLOW_MODES
{
    MODAL_FLOW_TEMPORARY_STOP = 0,                        // M0
    MODAL_FLOW_OPTIONAL_TEMP_STOP = 1,               // M1
    MODAL_FLOW_COMPLETED = 2,                   // M2
    
    MODAL_FLOW_COMPLETED_PALLET = 30,                       // M30
    
    MODAL_FLOW_DEFAULT_RUNNING              // 31, not M31
    
}GCODE_MODAL_PROG_FLOW_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_COOLANT_MODES
{
    MODAL_COOLANT_MIST = 7,                     // M7
    MODAL_COOLANT_FLOOD = 8,                    // M8
    MODAL_COOLANT_OFF = 9                       // M9
}GCODE_MODAL_COOLANT_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_SPINDLE_MODES
{
    MODAL_SPINDLE_CW = 3,                       // M3
    MODAL_SPINDLE_CCW = 4,                      // M4
    MODAL_SPINDLE_OFF = 5,                      // M5
    
    MODAL_SPINDLE_TOOL_CHANGE = 6,              // M6
}GCODE_MODAL_SPINDLE_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_CUTTER_RAD_COMP_MODES
{
    MODAL_CUTTER_RAD_COMP_OFF = 400,            // G40
    MODAL_CUTTER_RAD_COMP_LEFT = 410,           // G41
    MODAL_CUTTER_RAD_COMP_RIGHT = 420,          // G42
}GCODE_MODAL_CUTTER_RAD_COMP_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_TOOL_LEN_OFS_MODES
{
    MODAL_TOOL_LEN_OFS_PLUS = 430,              // G43
    MODAL_TOOL_LEN_OFS_CANCEL = 490,            // G49
}GCODE_MODAL_TOOL_LEN_OFS_MODES;

///////////////////////////////////////////////////////////////////////////////

// Define a bitmap of 32 bits with flags indicating the use of any value
// code word, for example X, Y, Z coordinates, etc.
//
// 0031 0030 0029 0028 0027 0026 0025 0024 0023 0022 0021 0020 0019 0018 0017 0016 

// 0015 0014 0013 0012 0011 0010 0009 0008 0007 0006 0005 0004 0003 0002 0001 0000
// ArcR Dwel LNum TIdx Diam Spin Feed ValK ValJ ValI ValC ValB ValA ValZ ValY ValX
#define VALUE_SET_X_BIT     (1 << 0)
#define VALUE_SET_Y_BIT     (1 << 1)
#define VALUE_SET_Z_BIT     (1 << 2)

#define VALUE_SET_A_BIT		(1 << 3)
#define VALUE_SET_B_BIT     (1 << 4)
#define VALUE_SET_C_BIT     (1 << 5)

#define VALUE_SET_I_BIT     (1 << 6)
#define VALUE_SET_J_BIT     (1 << 7)
#define VALUE_SET_K_BIT     (1 << 8)

#define VALUE_SET_F_BIT     (1 << 9)
#define VALUE_SET_S_BIT     (1 << 10)
#define VALUE_SET_D_BIT     (1 << 11)
#define VALUE_SET_T_BIT     (1 << 12)
#define VALUE_SET_L_BIT     (1 << 13)
#define VALUE_SET_P_BIT     (1 << 14)
#define VALUE_SET_R_BIT     (1 << 15)
#define VALUE_SET_Q_BIT     (1 << 16)
#define VALUE_SET_H_BIT     (1 << 17)

// Composite values (to check for errors)
#define VALUE_SET_XY_BITS   (VALUE_SET_X_BIT | VALUE_SET_Y_BIT)
#define VALUE_SET_XZ_BITS   (VALUE_SET_X_BIT | VALUE_SET_Z_BIT)
#define VALUE_SET_YZ_BITS   (VALUE_SET_Y_BIT | VALUE_SET_Z_BIT)

#define VALUE_SET_XYZ_BITS  (VALUE_SET_X_BIT | VALUE_SET_Y_BIT | VALUE_SET_Z_BIT)
#define VALUE_SET_ABC_BITS  (VALUE_SET_A_BIT | VALUE_SET_B_BIT | VALUE_SET_C_BIT)


#define VALUE_SET_ANY_AXES_BITS (VALUE_SET_XYZ_BITS | VALUE_SET_ABC_BITS)

#define VALUE_SET_IJ_BITS   (VALUE_SET_I_BIT | VALUE_SET_J_BIT)
#define VALUE_SET_IK_BITS   (VALUE_SET_I_BIT | VALUE_SET_K_BIT)
#define VALUE_SET_JK_BITS   (VALUE_SET_J_BIT | VALUE_SET_K_BIT)

#define VALUE_SET_IJK_BITS  (VALUE_SET_I_BIT | VALUE_SET_J_BIT | VALUE_SET_K_BIT)

///////////////////////////////////////////////////////////////////////////////

#define AXIS_COMMAND_TYPE_NONE              0
#define AXIS_COMMAND_TYPE_NON_MODAL         1
#define AXIS_COMMAND_TYPE_MOTION            2
#define AXIS_COMMAND_TYPE_TOOL_LEN_OFS      3
#define AXIS_COMMAND_TYPE_CANNED_CYCLE      4

///////////////////////////////////////////////////////////////////////////////

// Define a bitmap of 16 bits with flags indicating the use of any
// modal group (MGxx for G codes | MGMx for M codes)  on the current line
//
// 0015 0014 0013 0012 0011 0010 0009 0008 0007 0006 0005 0004 0003 0002 0001 0000
//  --  MGM9 MGM8 MGM7 MGM4 MG13 MG12 MG08 MG07 MG06 MG05 MG04 MG03 MG02 MG01 MG00
//
// Every time a code from any modal group is found, this bit is set. If already set
// this means multiple codes of the same modal group, which is a violation

#define MODAL_GROUP_G0_BIT (1 << 0)   // [G4,G10,G28,G28.1,G30,G30.1,G53,G92,G92.1] Non-modal
#define MODAL_GROUP_G1_BIT (1 << 1)   // [G0,G1,G2,G3,G38.2,G38.3,G38.4,G38.5,G80] Motion
#define MODAL_GROUP_G2_BIT (1 << 2)   // [G17,G18,G19] Plane selection
#define MODAL_GROUP_G3_BIT (1 << 3)   // [G90,G91] Distance mode
#define MODAL_GROUP_G4_BIT (1 << 4)   // [G91.1] Arc IJK distance mode
#define MODAL_GROUP_G5_BIT (1 << 5)   // [G93,G94] Feed rate mode
#define MODAL_GROUP_G6_BIT (1 << 6)   // [G20,G21] Units
#define MODAL_GROUP_G7_BIT (1 << 7)   // [G40] Cutter radius compensation mode. G41/42 NOT SUPPORTED.
#define MODAL_GROUP_G8_BIT (1 << 8)   // [G43.1,G49] Tool length offset
#define MODAL_GROUP_G10_BIT (1 << 9)  // [G98, G99] Return mode in canned cycles
#define MODAL_GROUP_G12_BIT (1 << 10)  // [G54,G55,G56,G57,G58,G59] Coordinate system selection
#define MODAL_GROUP_G13_BIT (1 << 11) // [G61] Control mode

#define MODAL_GROUP_M4_BIT (1 << 12)  // [M0,M1,M2,M30] Stopping
#define MODAL_GROUP_M6_BIT (1 << 13)  // [M6] Tool change
#define MODAL_GROUP_M7_BIT (1 << 14)  // [M3,M4,M5] Spindle turning
#define MODAL_GROUP_M8_BIT (1 << 15)  // [M7,M8,M9] Coolant control
#define MODAL_GROUP_M9_BIT (1 << 16)  // [M56] Override control

#define MODAL_GROUP_UNKNOWN 0

///////////////////////////////////////////////////////////////////////////////

// Non modal G-codes like G4, G10, G28, G28.1, G30, G30.1, G53, G92 & G92.1
#define NON_MODAL_DWELL                             40  // G4    (Do not alter value)
#define NON_MODAL_SET_COORDINATE_DATA               100 // G10   (Do not alter value)
#define NON_MODAL_GO_HOME_0                         280 // G28   (Do not alter value)
#define NON_MODAL_SET_HOME_0                        281 // G28.1 (Do not alter value)
#define NON_MODAL_GO_HOME_1                         300 // G30   (Do not alter value)
#define NON_MODAL_SET_HOME_1                        301 // G30.1 (Do not alter value)
#define NON_MODAL_ABSOLUTE_OVERRIDE                 530 // G53   (Do not alter value)
#define NON_MODAL_SET_COORDINATE_OFFSET_SAVE        920 // G92   (Do not alter value)
#define NON_MODAL_RESET_COORDINATE_OFFSET_SAVE      921 // G92.1 (Do not alter value)
#define NON_MODAL_RESET_COORDINATE_OFFSET_NO_SAVE   922 // G92.2 (Do not alter value)
#define NON_MODAL_APPLY_SAVED_COORDINATE_OFFSETS    923 // G92.3 (Do not alter value)


/*

G92 offset coordinate systems and set parameters
G92.1 cancel offset coordinate systems and set parameters to zero
G92.2 cancel offset coordinate systems but do not reset parameters
G92.3 apply parameters to offset coordinate systems

*/

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_COORD_SYS_SELECT_MODES
{
    MODAL_COORD_SYS_SELECT_0 = 0,
    MODAL_COORD_SYS_SELECT_1 = 540,             // G54
    MODAL_COORD_SYS_SELECT_2 = 550,             // G55
    MODAL_COORD_SYS_SELECT_3 = 560,             // G56
    MODAL_COORD_SYS_SELECT_4 = 570,             // G57
    MODAL_COORD_SYS_SELECT_5 = 580,             // G58
    MODAL_COORD_SYS_SELECT_6 = 590,             // G59
    MODAL_COORD_SYS_SELECT_7 = 591,             // G59.1
    MODAL_COORD_SYS_SELECT_8 = 592,             // G59.2
    MODAL_COORD_SYS_SELECT_9 = 593,             // G59.3
}GCODE_MODAL_COORD_SYS_SELECT_MODES;

///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_PATH_MODES
{
    MODAL_PATH_MODE_EXACT_PATH = 610,           // G61
    MODAL_PATH_MODE_EXACT_STOP = 611,           // G61.1
    MODAL_PATH_MODE_CONTINUOUS = 640            // G64

}GCODE_MODAL_PATH_MODES;
///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_MODAL_CANNED_RETURN_MODES
{
    MODAL_CANNED_RETURN_TO_PREV_POSITION = 980, // G98  [Return to Z]
    MODAL_CANNED_RETURN_TO_R_POSITION = 990     // G99  [Return to R]
}GCODE_MODAL_CANNED_RETURN_MODES;

///////////////////////////////////////////////////////////////////////////////


typedef struct GCodeModalData
{
    GCODE_MODAL_MOTION_MODES            motion_mode;
    GCODE_MODAL_FEEDRATE_MODES          feedrate_mode;
    GCODE_MODAL_UNITS_MODES             units_mode;
    GCODE_MODAL_DISTANCE_MODES          distance_mode;
    GCODE_MODAL_PLANE_SELECT_MODES      plane_select;
    GCODE_MODAL_PROG_FLOW_MODES         prog_flow;
    GCODE_MODAL_COOLANT_MODES           coolant_mode;
    GCODE_MODAL_SPINDLE_MODES           spindle_mode;
    GCODE_MODAL_CUTTER_RAD_COMP_MODES   cutter_comp_mode;
    GCODE_MODAL_TOOL_LEN_OFS_MODES      tool_len_ofs_mode;
    uint32_t							work_coord_sys_index;
    GCODE_MODAL_CANNED_RETURN_MODES     canned_return_mode;
    
}GCodeModalData;

///////////////////////////////////////////////////////////////////////////////

typedef struct GCodeBlockData
{
    // Block Non-modal Code
    int16_t non_modal_code;
    
    // Block Modal State
    GCodeModalData  block_modal_state;
    
    // Block values
    float coordinate_data[TOTAL_AXES_COUNT];

    float offset_ijk_data[COORDINATE_LINEAR_AXES_COUNT];
    
    float spindle_speed;
    float feed_rate;
    float tool_diameter;
    
    uint8_t tool_number;
    int16_t L_value;
    
    float P_value;
    float Q_value;
    float R_value;

    float block_coordinate_origin[TOTAL_AXES_COUNT];
    
    int32_t D_value;
    int32_t H_value;
    
}GCodeBlockData;

///////////////////////////////////////////////////////////////////////////////
#include "Planner.h"
///////////////////////////////////////////////////////////////////////////////

class Planner;

class GCodeParser
{
public:
        GCodeParser();
        ~GCodeParser();

        void AssociatePlanner(Planner* planner) { m_planner_ref = planner; }
    
        void ResetParser();
        int ParseLine(char* line);

        inline void EnableCheckMode() { m_check_mode = true; }
        inline void DisableCheckMode() { m_check_mode = false; } 
        inline bool IsCheckModeActive() { return m_check_mode; } 

        static const char*  GetErrorText(uint32_t error_code);
        
        void ReadParserModalState(GCodeModalData* block) const;
        inline const float* ReadOriginCoords() const { return m_work_coord_sys; } 
        inline const float* ReadOffsetCoords() const { return m_g92_coord_offset; } 
        
    protected:
        char*           m_line;
    
        GCodeModalData  m_parser_modal_state;
        
        float           m_spindle_speed;
        float           m_feed_rate;
        uint8_t         m_tool_number;

        uint8_t         m_axis_zero;
		uint8_t         m_axis_one;
		uint8_t         m_axis_linear;

        uint32_t        m_value_group_flags;
		uint8_t         m_axis_command_type;

        // Active Working coordinate system (selected with G54, G55, ..., G59.3) work_coord_sys_index @ modal params
        float           m_work_coord_sys[TOTAL_AXES_COUNT];

        float           m_g92_coord_offset[TOTAL_AXES_COUNT];
        float           m_tool_offset[COORDINATE_LINEAR_AXES_COUNT];
        float           m_last_probe_position[TOTAL_AXES_COUNT];
        float           m_gcode_machine_pos[TOTAL_AXES_COUNT];
        
        float           m_soft_limit_max_values[TOTAL_AXES_COUNT];
        bool            m_soft_limit_enabled[TOTAL_AXES_COUNT];

        GCodeBlockData  m_block_data;
        
        bool            m_check_mode;
        
        Planner*        m_planner_ref;  
        
        float           m_mm_per_arc_segment;           
        float           m_mm_max_arc_error;             
        uint8_t         m_arc_correction_counter;
        
        // State Information for Canned Cycles
        bool            m_canned_cycle_active;
        
        float           m_cc_initial_z;     // Initial Z value before canned cycle
        float           m_cc_r_plane;       // R position to return
        float           m_cc_sticky_z;         // Final depth
        float           m_cc_sticky_r;         // R plane
        float           m_cc_sticky_f;         // Feedrate
        float           m_cc_sticky_q;         // Depth increment
        float           m_cc_sticky_p;         // Dwell pause [secs]

        ///////////////////////////////////////////////////////////////////////////////////////////

        int     cleanup_and_lowercase_line(uint32_t & line_len);
        float   convert_to_mm(float value);
        void    reset_modal_params();

        int     check_codes_using_axes();
        int     check_group_0_codes();
        int     check_unused_codes();

        void    handle_coordinate_system_select();
        int     handle_non_modal_codes();
        int     handle_motion_commands();
        
        int     motion_append_line(const float * target_pos);
        
        void    canned_cycle_reset_stycky();
        void    canned_cycle_update_sticky();
        
};

#endif
