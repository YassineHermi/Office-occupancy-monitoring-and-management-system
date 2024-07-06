/*
 * app_nvm.h file for offices data storing
 *
 * Author : Yassine HERMI
 * Date : 18/06/2024
 *
 */

#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "app_error.h"
#include <string.h>
#include <stdbool.h>

#define OFFICE_COUNT             6 
#define FLASH_START_ADDRESS      0x78000 

typedef struct {
    char office_Id[8];
    uint8_t availability;
    char employee_name[27];
} office_item;


static office_item Offices_Registry[] =
{
    /* ID                   Availability             employee_name   */
    {"E1B2R2P2",                        1,             "Bilel"      },
    {"E1B2R3P2",                        1,             "Yassine"    },
    {"E1B2R4P2",                        1,             "Sofiene"    },
    {"E1B3R1P2",                        1,             "Sabri"      },
    {"E1B3R3P2",                        1,             "Hamza"      },
    {"E1B3R4P2",                        1,             "Imed"       },
};


/**@brief Function for initializing the flash storage library.
 */
void flash_storage_init(void);

/**@brief Function for erasing a page from flash starting from FLASH_START_ADDRESS.
 */
void erase_office_table_from_flash(void);

/**@brief Function for reading offices data from flash.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 */
void read_office_table_from_flash(office_item office_table[OFFICE_COUNT]);

/**@brief Function for writing offices data in flash.
 *
 * @param[in]   office_table       table containing offices data that will be stored in flash.
 */
void write_office_table_to_flash(office_item office_table[OFFICE_COUNT]);

/**@brief Function for reserving an office for an employee.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id that will be reserved.
 * @param[in]   employee_name      pointer to the employee name.
 */
void reserve_office(office_item office_table[OFFICE_COUNT], char* office_id, char* employee_name);

/**@brief Function for clearing an office.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id that will be cleared.
 */
void clear_office(office_item office_table[OFFICE_COUNT], char* office_id);

/**@brief Function for returing an office occupancy.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id.
 *
 * @return      true if reserved, false if available.
 */
bool is_office_available(office_item office_table[OFFICE_COUNT], char* office_id);

/**@brief Function for returning an employee name for a given office id.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id.
 *
 * @return      the employee name.
 */
char* get_employee_name_for_office(office_item office_table[OFFICE_COUNT], const char *office_id);

/**@brief Function for returing if an office exists or not.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id.
 *
 * @return      true if found, false if not.
 */
bool does_office_exist(office_item office_table[OFFICE_COUNT], const char *office_id);