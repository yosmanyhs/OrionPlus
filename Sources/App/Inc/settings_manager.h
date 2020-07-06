#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <stdint.h>

/*
 * signal_invert_mask has one bit for every stepper motor control signal
 * 
 *  Zero means the signal is high level active (no invert)
 *  One means the signal is low level active (inverted)
 * 
 *  The assignment is as follows
 *
 *   15    14    13    12    11    10    09    08  
 *  Enab  Rset  GFlt                          LimZ            
 *
 *   07    06    05    04    03    02    01    00
 *  LimY  LimX  DirX  StpX  DirY  StpY  DirZ  StpZ
 */
#define SIGNAL_INVERT_STEP_Z            (1 << 0)
#define SIGNAL_INVERT_DIR_Z             (1 << 1)
#define SIGNAL_INVERT_STEP_Y            (1 << 2)
#define SIGNAL_INVERT_DIR_Y             (1 << 3)
#define SIGNAL_INVERT_STEP_X            (1 << 4)
#define SIGNAL_INVERT_DIR_X             (1 << 5)

#define SIGNAL_INVERT_STEP_PINS_MASK    ((1 << 4) | (1 << 2) | (1 << 0))    // 00010101 = 0x15
#define SIGNAL_INVERT_DIR_PINS_MASK     ((1 << 5) | (1 << 3) | (1 << 1))    // 00101010 = 0x2A

#define SIGNAL_INVERT_LIM_X             (1 << 6)
#define SIGNAL_INVERT_LIM_Y             (1 << 7)
#define SIGNAL_INVERT_LIM_Z             (1 << 8)

#define SIGNAL_INVERT_LIMIT_PINS_MASK   ((1 << 8) | (1 << 7) | (1 << 6))

#define SIGNAL_INVERT_GLB_FLT           (1 << 13)
#define SIGNAL_INVERT_STP_RST           (1 << 14)
#define SIGNAL_INVERT_STP_ENA           (1 << 15)

#pragma pack(1)
typedef union STEPPER_CONTROL_DATA
{
    uint32_t AsWord32;
    
    struct
    {
        uint16_t    signal_invert_mask; // Bitmap of inversion masks for signals
        uint8_t     step_pulse_len_us;  // Duration of high level on step's pulses
        uint8_t     step_idle_lock_secs;  // Time to keep enabled stepper motos after stop
    }S;
}STEPPER_CONTROL_DATA;

typedef struct SETTINGS_DATA
{
    uint32_t    settings_header;
    
    union STEPPER_CONTROL_DATA step_ctrl;
    
    float       junction_deviation_mm;  
    float       arc_tolerance_mm;
    
    float       home_seek_rate_mm_sec;
    float       home_feed_rate_mm_sec;
    uint32_t    home_debounce_ms;
    float       home_pull_off_distance_mm;

    float       steps_per_mm_axes[3];
    float       max_travel_mm_axes[3];
    float       max_rate_mm_sec_axes[3];
    float       accel_mm_sec2_axes[3];  // 1mm/s2 = 3600mm/min2
    
    float       steps_per_deg_axes[3];
    float       max_rate_deg_sec_axes[3];
    float       accel_deg_sec2_axes[3];
    
    float       spindle_min_rpm;
    float       spindle_max_rpm;
    
    uint32_t    active_coord_system;
    float       aux_coord_systems[9][6];
    
    float       g28_position[6];
    float       g30_position[6];
	float		g92_offsets[6];
    
    // TODO: Add another uint32_t to contain boolean variables like soft_endstop_enables, etc.
    
    uint32_t    settings_crc;

}SETTINGS_DATA;

#pragma pack()

#ifndef SETTINGS_DATA_SIZE_BYTES
    #define SETTINGS_DATA_SIZE_BYTES sizeof(SETTINGS_DATA)
#endif
    
#define SETTINGS_DATA_SIZE_WORDS_NO_CRC     ((sizeof(SETTINGS_DATA)/sizeof(uint32_t)) - 1)
    
#define SETTINGS_DATA_START_ADDRESS     0x00000000
#define SETTINGS_HEADER_VALUE           0x7A534859  // YHSz



class Settings_Manager
{
public:

    static void Initialize();
	static int Load();
	static void Save();

	static void ResetToDefaults();

	static int ReadCoordinateValues(uint32_t coord_index, float * buffer);
	static int WriteCoordinateValues(uint32_t coord_index, const float * buffer);

    static inline uint16_t GetSignalInversionMasks() { return m_data->step_ctrl.S.signal_invert_mask; }
    static inline uint32_t GetIdleLockTime_secs() { return m_data->step_ctrl.S.step_idle_lock_secs; }
    static inline uint32_t GetPulseLenTime_us() { return m_data->step_ctrl.S.step_pulse_len_us; }

    static void SetActiveCoordinateSystemIndex(uint32_t index) { m_data->active_coord_system = index; }
    
    static bool AreSoftLimitsEnabled() { return m_soft_endstops_enabled; }
    static void EnableSoftLimits() { m_soft_endstops_enabled = true; }
    static void DisableSoftLimits() { m_soft_endstops_enabled = false; }
    
    static inline float GetMaxSpeed_mm_sec_axis(uint32_t axis) { return (m_data->max_rate_mm_sec_axes[axis]); }
    static inline const float* GetMaxSpeed_mm_sec_all_axes() { return (m_data->max_rate_mm_sec_axes); }
    
    static inline void SetMaxSpeed_mm_sec_axis(uint32_t axis, float value) { m_data->max_rate_mm_sec_axes[axis] = value; }
    
    static inline float GetAcceleration_mm_sec2_axis(uint32_t axis) { return (m_data->accel_mm_sec2_axes[axis]); }
    static inline const float* GetAcceleration_mm_sec2_all_axes() { return (m_data->accel_mm_sec2_axes); }
    
    static inline void SetAcceleration_mm_sec2_axis(uint32_t axis, float value) { m_data->accel_mm_sec2_axes[axis] = value; }
    
    static inline float GetJunctionDeviation_mm() { return m_data->junction_deviation_mm; }
    static inline void  SetJunctionDeviation_mm(float jd_value) { m_data->junction_deviation_mm = jd_value; }
    
    static inline float GetStepsPer_mm_Axis(uint32_t ax_idx) { return m_data->steps_per_mm_axes[ax_idx % 3]; }
    static inline void  SetStepsPer_mm_Axis(uint32_t ax_idx, float st_mm) { m_data->steps_per_mm_axes[ax_idx % 3] = st_mm; } 
    
protected:
    
    static void Internal_AllocMemory(void);

	static SETTINGS_DATA * m_data;

    static bool m_soft_endstops_enabled;
};

#endif
