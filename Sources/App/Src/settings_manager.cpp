#include <stm32f4xx_hal.h>

#include <string.h>

#include "FreeRTOS.h"

#include "spi_ports.h"
#include "settings_manager.h"


static CRC_HandleTypeDef   CrcHandle;

void HAL_CRC_MspInit(CRC_HandleTypeDef *hcrc)
{
    __HAL_RCC_CRC_CLK_ENABLE();
}

// Define m_data as a static member of Settings_Manager class
SETTINGS_DATA* Settings_Manager::m_data;

void Settings_Manager::Initialize()
{
    CrcHandle.Instance = CRC;
    HAL_CRC_Init(&CrcHandle);
    
    Internal_AllocMemory();
    
    if (Load() != 0)
    {
        // Error loading settings. Already returned to defaults. 
        Save();
    }
}

int Settings_Manager::Load()
{
    SETTINGS_DATA * pData;
    uint32_t read_data_crc;    
    
    pData = (SETTINGS_DATA*)pvPortMalloc(SETTINGS_DATA_SIZE_BYTES);
    
    if (pData != NULL)
    {
        // Try to read data from flash memory
        W25QXX_Read((uint8_t*)pData, SETTINGS_DATA_START_ADDRESS, SETTINGS_DATA_SIZE_BYTES);
        
        // Calculate read data CRC and check against retrieved value
        read_data_crc = HAL_CRC_Calculate(&CrcHandle, (uint32_t*)pData, SETTINGS_DATA_SIZE_WORDS_NO_CRC);
        
        if ((read_data_crc != pData->settings_crc) ||
           (pData->settings_header != SETTINGS_HEADER_VALUE)) 
        {
            // Mismatch, release memory and return error
            vPortFree((void*)pData);
            ResetToDefaults();
            return 1;
        }
        
        // Check other values for inconsistencies (not implemented yet)
        
        // Copy data to settings
        memcpy((void*)m_data, (const void*)pData, SETTINGS_DATA_SIZE_BYTES);
        
        // Release memory
        vPortFree((void*)pData);
        
        // and exit successfully
        return 0;        
    }
    
    // In any other case, return error.
    ResetToDefaults();
    return 1;
}

void Settings_Manager::Save()
{
    const uint32_t FLASH_PAGE_SIZE = 256;
    
    uint32_t bytes_to_write;
    uint32_t new_crc = HAL_CRC_Calculate(&CrcHandle, (uint32_t*)m_data, SETTINGS_DATA_SIZE_WORDS_NO_CRC);
    uint32_t addr = SETTINGS_DATA_START_ADDRESS;
    uint32_t total_len = SETTINGS_DATA_SIZE_BYTES;
    uint8_t * data_ptr = (uint8_t*)(m_data);
    
    // Only if something changed then update flash    
    if (new_crc != m_data->settings_crc)
    {
        // Update settings CRC, erase sector (4K) and write settings to flash
        m_data->settings_crc = new_crc;
    
        // Erase sector
        W25QXX_Erase_Sector(SETTINGS_DATA_START_ADDRESS);
        
        while (total_len != 0)
        {    
            // Restrict data count to max page size
            if (total_len > FLASH_PAGE_SIZE)
                bytes_to_write = FLASH_PAGE_SIZE;
            else
                bytes_to_write = total_len;
            
            // Write data page (up to 256 bytes)
            W25QXX_Write_Page(data_ptr, addr, bytes_to_write);
            
            // Update address, data pointer & remaining length
            total_len -= bytes_to_write;
            addr += bytes_to_write;
            data_ptr += bytes_to_write;
        }
    }
}

void Settings_Manager::ResetToDefaults()
{    
	memset(m_data, 0, SETTINGS_DATA_SIZE_BYTES);

	m_data->settings_header = SETTINGS_HEADER_VALUE;
    
    m_data->step_ctrl.AsWord32 = 0x00000000;
    
    m_data->step_ctrl.S.step_pulse_len_us = 10;
    m_data->step_ctrl.S.step_idle_lock_secs = 30;
    
    m_data->junction_deviation_mm = 0.002f;
    m_data->arc_segment_size_mm = 0.0f;
    m_data->max_arc_error_mm =  0.01f;
    
    
    m_data->homing_data.home_seek_rate_mm_sec = 12.50f; // 750 mm/min -> 750/60 mm/sec = 12.5
    m_data->homing_data.home_feed_rate_mm_sec = 7.50f; // 450 mm/min -> 450/60 mm/sec = 7.5
    m_data->homing_data.home_debounce_ms = 5;
    m_data->homing_data.home_pull_off_distance_mm = 1.0f;
    
    // Microstep = 1/4; 4mm/rev; 4*200 steps/rev => 800/4 = 200 steps/mm
    m_data->steps_per_mm_axes[0] = 200.0f;
    m_data->steps_per_mm_axes[1] = 200.0f;
    m_data->steps_per_mm_axes[2] = 200.0f;
    
    // Microstep = 1/4; 360deg/rev; 1.8deg/4 per step; 1 deg = 4/1.8 steps = 2.2222...
    m_data->steps_per_deg_axes[0] = 2.22f;
    m_data->steps_per_deg_axes[1] = 2.22f;
    m_data->steps_per_deg_axes[2] = 2.22f;
    
    m_data->max_travel_mm_axes[0] = 360.0f; // 365
    m_data->max_travel_mm_axes[1] = 360.0f; // 365
    m_data->max_travel_mm_axes[2] = 119.0f; // 119.5
    
    m_data->max_rate_mm_sec_axes[0] = 20.0f;  // 1200 mm/min / 60 -> 20 mm/sec -> 20/4 -> 5 rps * 800 = 4000 kHz 
    m_data->max_rate_mm_sec_axes[1] = 20.0f;
    m_data->max_rate_mm_sec_axes[2] = 20.0f;
    
    m_data->accel_mm_sec2_axes[0] = 2.0f;  // 120 mm/min2 / 3600 -> 0.03333 mm/sec2
    m_data->accel_mm_sec2_axes[1] = 2.0f;
    m_data->accel_mm_sec2_axes[2] = 2.0f;
    
    m_data->spindle_min_rpm = 0.0f;
    m_data->spindle_max_rpm = 10000.0f;
    
    m_data->bit_settings.Bits.soft_limits_enabled = true;
    m_data->bit_settings.Bits.soft_limit_x_enable = true;
    m_data->bit_settings.Bits.soft_limit_y_enable = true;
    m_data->bit_settings.Bits.soft_limit_z_enable = true;
}

// Buffer variable must have space to copy 6 float values
// in the order X, Y, Z, A, B, C
int Settings_Manager::ReadCoordinateValues(uint32_t coord_index, float * buffer)
{
	// Check coordinate system index value [0 .. 9, 28, 30, 92]
	switch (coord_index)
	{
		case 28:
			memcpy(buffer, m_data->g28_position, sizeof(m_data->g28_position));
			break;

		case 30:
			memcpy(buffer, m_data->g30_position, sizeof(m_data->g30_position));
			break;

		case 92:
			memcpy(buffer, m_data->g92_offsets, sizeof(m_data->g92_offsets));
			break;

		default:
		{
			if (coord_index > 8)
				return -1;

			memcpy(buffer, m_data->aux_coord_systems[coord_index], sizeof(m_data->aux_coord_systems[0]));
		}
		break;
	}

	return 0;
}


// Buffer variable must contain 6 float values
// in the order X, Y, Z, A, B, C
int Settings_Manager::WriteCoordinateValues(uint32_t coord_index, const float * buffer)
{
	// Check coordinate system index value [0 .. 9, 28, 30, 92]
	switch (coord_index)
	{
		case 28:
			memcpy(m_data->g28_position, buffer, sizeof(m_data->g28_position));
			break;

		case 30:
			memcpy(m_data->g30_position, buffer, sizeof(m_data->g30_position));
			break;

		case 92:
			memcpy(m_data->g92_offsets, buffer, sizeof(m_data->g92_offsets));
			break;

		default:
		{
			if (coord_index > 8)
				return -1;

			memcpy(m_data->aux_coord_systems[coord_index], buffer, sizeof(m_data->aux_coord_systems[0]));
		}
		break;
	}

	return 0;
}

void Settings_Manager::SetHomingData(const HOMING_SETUP_DATA& updated_data)
{
    m_data->homing_data.home_seek_rate_mm_sec = updated_data.home_seek_rate_mm_sec;
    m_data->homing_data.home_feed_rate_mm_sec = updated_data.home_feed_rate_mm_sec;
    m_data->homing_data.home_debounce_ms = updated_data.home_debounce_ms;
    m_data->homing_data.home_pull_off_distance_mm = updated_data.home_pull_off_distance_mm;
}

void Settings_Manager::Internal_AllocMemory(void)
{
    if (NULL == m_data)
    {
        m_data = (SETTINGS_DATA*)pvPortMalloc(SETTINGS_DATA_SIZE_BYTES);
        memset(m_data, 0, SETTINGS_DATA_SIZE_BYTES);
    }
}

