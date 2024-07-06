/*
 * app_nvm.c file for offices data storing
 *
 * Author : Yassine HERMI
 * Date : 18/06/2024
 *
 */

#include "app_nvm.h"
#include "nrf_log.h"
#include "nrf_soc.h"
#include "stdlib.h"
#include "nrf_fstorage.h"
#include <string.h>
#include <stdbool.h>
#include "nrf_delay.h"
   

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);
static void print_flash_info(nrf_fstorage_t * p_fstorage);
static void power_manage(void);
static void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage);



NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = FLASH_START_ADDRESS,
    .end_addr   = 0x79fff,
};



static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}


static void print_flash_info(nrf_fstorage_t * p_fstorage)
{
    NRF_LOG_INFO("========| flash info |========");
    NRF_LOG_INFO("erase unit: \t%d bytes",      p_fstorage->p_flash_info->erase_unit);
    NRF_LOG_INFO("program unit: \t%d bytes",    p_fstorage->p_flash_info->program_unit);
    NRF_LOG_INFO("==============================");
}

/**@brief   Sleep until an event is received. */
static void power_manage(void)
{
#ifdef SOFTDEVICE_PRESENT
    (void) sd_app_evt_wait();
#else
    __WFE();
#endif
}

static void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        power_manage();
    }
}

/**@brief Function for initializing the flash storage library.
 */
void flash_storage_init(void)
{
    ret_code_t rc;
    nrf_fstorage_api_t * p_fs_api;
    office_item read_table[OFFICE_COUNT];

#ifdef SOFTDEVICE_PRESENT
    /*NRF_LOG_INFO("SoftDevice is present.");
    NRF_LOG_INFO("Initializing nrf_fstorage_sd implementation...");*/
    /* Initialize an fstorage instance using the nrf_fstorage_sd backend.
     * nrf_fstorage_sd uses the SoftDevice to write to flash. This implementation can safely be
     * used whenever there is a SoftDevice, regardless of its status (enabled/disabled). */
    p_fs_api = &nrf_fstorage_sd;
#else
    /*NRF_LOG_INFO("SoftDevice not present.");
    NRF_LOG_INFO("Initializing nrf_fstorage_nvmc implementation...");*/
    /* Initialize an fstorage instance using the nrf_fstorage_nvmc backend.
     * nrf_fstorage_nvmc uses the NVMC peripheral. This implementation can be used when the
     * SoftDevice is disabled or not present.
     *
     * Using this implementation when the SoftDevice is enabled results in a hardfault. */
    p_fs_api = &nrf_fstorage_nvmc;
#endif

    rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);

    print_flash_info(&fstorage);
    
    //erase_office_table_from_flash();
    read_office_table_from_flash(read_table);
    if (strncmp(read_table[0].office_Id, Offices_Registry[0].office_Id, sizeof(Offices_Registry[0].office_Id)) != 0) {
        NRF_LOG_INFO("No data stored\n\n");
        write_office_table_to_flash(Offices_Registry);
    }
     
}

/**@brief Function for writing offices data in flash.
 *
 * @param[in]   office_table       table containing offices data that will be stored in flash.
 */
void write_office_table_to_flash(office_item office_table[OFFICE_COUNT]) 
{
    ret_code_t rc;

    rc = nrf_fstorage_write(&fstorage, FLASH_START_ADDRESS, office_table, OFFICE_COUNT*sizeof(office_table[0]), NULL);
    APP_ERROR_CHECK(rc);
    
    wait_for_flash_ready(&fstorage);
    NRF_LOG_INFO("Done.");
}

/**@brief Function for reading offices data from flash.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 */
void read_office_table_from_flash(office_item office_table[OFFICE_COUNT]) 
{
    ret_code_t rc;
    rc = nrf_fstorage_read(&fstorage, FLASH_START_ADDRESS, office_table, OFFICE_COUNT*sizeof(office_table[0]));
    APP_ERROR_CHECK(rc);
}

/**@brief Function for erasing a page from flash starting from FLASH_START_ADDRESS.
 */
void erase_office_table_from_flash(void) 
{
    ret_code_t rc;
    rc = nrf_fstorage_erase(&fstorage, fstorage.start_addr, 1, NULL);
    APP_ERROR_CHECK(rc);
}

/**@brief Function for reserving an office for an employee.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id that will be reserved.
 * @param[in]   employee_name      pointer to the employee name.
 */
void reserve_office(office_item office_table[OFFICE_COUNT], char* office_id, char* employee_name) 
{
    for (int i = 0; i < OFFICE_COUNT; i++) 
    {
        if (strncmp(office_table[i].office_Id, office_id, sizeof(office_table[i].office_Id)) == 0) 
        {
            //NRF_LOG_INFO("office found");
            office_table[i].availability = 1; // 1 for reserved
            strcpy(office_table[i].employee_name, employee_name); 
            break;
        }
    }
}

/**@brief Function for clearing an office.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id that will be cleared.
 */
void clear_office(office_item office_table[OFFICE_COUNT], char* office_id) 
{
    for (int i = 0; i < OFFICE_COUNT; i++) 
    {
        if (strncmp(office_table[i].office_Id, office_id, sizeof(office_table[i].office_Id)) == 0) 
        {
            //NRF_LOG_INFO("office found");
            office_table[i].availability = 0; // 0 for available
            strcpy(office_table[i].employee_name, NULL); 
            break;
        }
    }
}

/**@brief Function for returing an office occupancy.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id.
 *
 * @return      true if reserved, false if available.
 */
bool is_office_available(office_item office_table[OFFICE_COUNT], char* office_id) 
{
    for (int i = 0; i < OFFICE_COUNT; i++) 
    {
        if (strncmp(office_table[i].office_Id, office_id, sizeof(office_table[i].office_Id)) == 0) 
        {
            return office_table[i].availability == 1; // Return true if reserved
        }
    }
    return false; 
}

/**@brief Function for returning an employee name for a given office id.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id.
 *
 * @return      the employee name.
 */
char* get_employee_name_for_office(office_item office_table[OFFICE_COUNT], const char *office_id)
{
    for (int i = 0; i < OFFICE_COUNT; i++)
    {
        if (strncmp(office_table[i].office_Id, office_id, sizeof(office_table[i].office_Id)) == 0)
        {
            if (office_table[i].availability == 1) // Office is reserved
            {
                return office_table[i].employee_name;
            }
            else
            {
                return "Office is not reserved"; // Indicate that the office is not reserved
            }
        }
    }
    return "Office not found"; // Indicate that the office ID was not found in the table
}

/**@brief Function for returing if an office exists or not.
 *
 * @param[in]   office_table       table containing offices data stored in flash.
 * @param[in]   office_id          pointer to the office id.
 *
 * @return      true if found, false if not.
 */
bool does_office_exist(office_item office_table[OFFICE_COUNT], const char *office_id)
{
    for (int i = 0; i < OFFICE_COUNT; i++)
    {
        if (strncmp(office_table[i].office_Id, office_id, sizeof(office_table[i].office_Id)) == 0)
        {
            return true;
        }
    }
    return false;
}