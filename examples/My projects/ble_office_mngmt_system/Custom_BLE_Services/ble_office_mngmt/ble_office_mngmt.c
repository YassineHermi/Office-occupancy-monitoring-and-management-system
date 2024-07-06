
/*
 * ble_office_mngmt.c file for the ble_office_mngmt custom service
 *
 * Author : Yassine HERMI
 * Date : 18/06/2024
 *
 */

#include "ble_office_mngmt.h"
#include "nrf_log.h"
#include "stdlib.h"
#include "app_nvm.h"

#define     Office_Id_Size         8
   
char ** words;
Memory_Action action;


/**@brief Function for splitting a strinf into words.
 *
 * @param[in]   str                pointer to the string to be splitted.
 * @param[in]   word_count         pointer to the number of words in the string.
 *
 * @return      Array containing the words.
 */
static char** split_string(uint8_t* str, uint8_t * word_count) 
{
    // Allocate memory for the array of words
    char** words = (char**)malloc(3 * sizeof(char*));
    *word_count = 0;
    char* token = strtok((char*)str, " ");
    
    while (token != NULL && *word_count < 3) 
    {
        words[*word_count] = (char*)malloc((strlen(token) + 1) * sizeof(char));
        strcpy(words[*word_count], token);
        (*word_count)++;
        token = strtok(NULL, " ");
    }

    return words;
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_cus       Custom service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    ble_cus_evt_t                 evt;
    uint8_t                       word_count;
    office_item                   read_table[OFFICE_COUNT];
    
    // writing to the office managing characteristic
   if (p_evt_write->handle == p_cus->office_managing_char_handles.value_handle)
    { 
        read_office_table_from_flash(read_table);
        words = split_string((uint8_t *)p_evt_write->data, &word_count);
        /*NRF_LOG_INFO("words[0] = %s", words[0]);
        NRF_LOG_INFO("Number of words written : %d", word_count);*/
        if((strncmp(words[0],"Free", sizeof("Free")) == 0)&&(word_count==2))
        {

            action = Clear_Office;
            if (does_office_exist(read_table, words[1]))
            {
                char value_to_be_written[Office_Id_Size + sizeof(" is cleared")-1];
                    
                memset(value_to_be_written, 0, sizeof(value_to_be_written));
                strcpy(value_to_be_written, words[1]);
                strcat(value_to_be_written, " is cleared");
                //NRF_LOG_INFO("value to be written : %s", value_to_be_written);
                NRF_LOG_INFO("%s is cleared", words[1]);
                evt.params_command.command_data.p_data = (uint8_t *)value_to_be_written;
                evt.params_command.command_data.length = sizeof(value_to_be_written);
            }
            else
            {
                evt.params_command.command_data.p_data = (uint8_t *)"Office not found";
                evt.params_command.command_data.length = sizeof("Office not found");
            }

        }
        else if((strncmp(words[0],"Reserve", sizeof("Reserve")) == 0)&&(word_count==3))
        {

            action = Reserve_Office;
            if (does_office_exist(read_table, words[1]))
            {
                char value_to_be_written[Office_Id_Size + sizeof(" is reserved")-1];
                    
                strcpy(value_to_be_written, words[1]);
                strcat(value_to_be_written, " is reserved");
                //NRF_LOG_INFO("value to be written : %s", value_to_be_written);
                NRF_LOG_INFO("%s is reserved", words[1]);
                evt.params_command.command_data.p_data = (uint8_t *)value_to_be_written;
                evt.params_command.command_data.length = sizeof(value_to_be_written);
            }
            else
            {
                evt.params_command.command_data.p_data = (uint8_t *)"Office not found";
                evt.params_command.command_data.length = sizeof("Office not found");
            }

        }
        else
        {
            action = Monitor_Office;
            if (does_office_exist(read_table, words[0]))
            {
                char * employee_name = get_employee_name_for_office(read_table, words[0]);
                char value_to_be_written[17 + sizeof("Reserved for ")-1];
                strcpy(value_to_be_written, "Reserved for ");
                strcat(value_to_be_written, employee_name);
                NRF_LOG_INFO("%s occupancy : %d", words[0], is_office_available(read_table, words[0]));
                evt.params_command.command_data.p_data = (is_office_available(read_table, words[0]) == 1) ? (uint8_t *)value_to_be_written : (uint8_t *)"Available";
                evt.params_command.command_data.length = (is_office_available(read_table, words[0]) == 1) ? sizeof(value_to_be_written) : sizeof("Available");
            }
            else
            {
                evt.params_command.command_data.p_data = (uint8_t *)"Office not found";
                evt.params_command.command_data.length = sizeof("Office not found");
            }
        }

        evt.evt_type = BLE_OFFICE_MANAGING_CHAR_EVT_WRITE; 

        p_cus->evt_handler(p_cus, &evt);
        
    }

    // writing to the office monitoring characteristic (cccd) "client characteristic configuration descriptor"
   else if (p_evt_write->handle == p_cus->office_monitoring_char_handles.cccd_handle)
   {
      if (ble_srv_is_notification_enabled(p_evt_write->data))
      {
          evt.evt_type = BLE_OFFICE_MONITORING_CHAR_NOTIFICATIONS_ENABLED;
      }
      else
      {
          evt.evt_type = BLE_OFFICE_MONITORING_CHAR_NOTIFICATIONS_DISABLED;
      }

      p_cus->evt_handler(p_cus, &evt);
   }

}

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

    ble_cus_evt_t evt;

    evt.evt_type = BLE_CUS_EVT_CONNECTED;

    p_cus->evt_handler(p_cus, &evt);
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;
    
    ble_cus_evt_t evt;

    evt.evt_type = BLE_CUS_EVT_DISCONNECTED;

    p_cus->evt_handler(p_cus, &evt);
}

/**@brief Function for handling the Custom servie ble events.
 *
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
void ble_cus_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_cus_t * p_cus = (ble_cus_t *) p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            on_write(p_cus, p_ble_evt);
            break;
        
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_cus, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_cus, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for initializing the Custom ble service.
 *
 * @param[in]   p_cus       Custom service structure.
 * @param[in]   p_cus_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */

uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{

    uint32_t                  err_code;
    ble_uuid_t                ble_uuid;
    ble_add_char_params_t     add_char_params;
    ble_add_descr_params_t    add_descr_params;

/* Adding the service */

    // Initialize service structure.
    p_cus->evt_handler               = p_cus_init->evt_handler;
    p_cus->conn_handle               = BLE_CONN_HANDLE_INVALID;

    // Add the Custom ble Service UUID
    ble_uuid128_t base_uuid =  CUS_SERVICE_UUID_BASE;
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = CUS_SERVICE_UUID;

    // Add the service to the database
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

/* Adding the characteristics */
    
    // Add the office managing characteristic.

    uint8_t leds_char_init_value [30] = {0};

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid             = OFFICE_MANAGING_CHAR_UUID;
    add_char_params.uuid_type        = p_cus->uuid_type;

    add_char_params.init_len         = 30; // (in bytes)
    add_char_params.max_len          = 30;
   add_char_params.p_init_value      = leds_char_init_value;

    add_char_params.char_props.read  = 1;
    add_char_params.char_props.write = 1;

    add_char_params.read_access  = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_params, 
                                  &p_cus->office_managing_char_handles);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    memset(&add_descr_params, 0, sizeof(add_descr_params));
    add_descr_params.uuid = BLE_UUID_DESCRIPTOR_CHAR_USER_DESC;
    add_descr_params.uuid_type         = p_cus->uuid_type;
    add_descr_params.init_len = 20;
    add_descr_params.max_len = 20;
    add_descr_params.read_access = SEC_OPEN;
    add_descr_params.p_value = (uint8_t *)"Office Managing ";
    err_code = descriptor_add((p_cus->office_managing_char_handles).value_handle,
                              &add_descr_params,
                              &p_cus->office_managing_char_handles.user_desc_handle);
                              
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    // Add office monitoring characteristic.

    uint8_t char_init_value [30] = {0};

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid             = OFFICE_MONITORING_CHAR_UUID;
    add_char_params.uuid_type        = p_cus->uuid_type;

    add_char_params.init_len         = 30; // (in bytes)
    add_char_params.max_len          = 30;
    add_char_params.p_init_value      = char_init_value;

    add_char_params.char_props.read  = 1;
    //add_char_params.char_props.write = 1;
    add_char_params.char_props.notify = 1;

    add_char_params.read_access  = SEC_OPEN;
    //add_char_params.write_access = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;

        err_code = characteristic_add(p_cus->service_handle, 
                                  &add_char_params, 
                                  &p_cus->office_monitoring_char_handles);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    memset(&add_descr_params, 0, sizeof(add_descr_params));
    add_descr_params.uuid = BLE_UUID_DESCRIPTOR_CHAR_USER_DESC;
    add_descr_params.uuid_type         = p_cus->uuid_type;
    add_descr_params.init_len = 20;
    add_descr_params.max_len = 20;
    add_descr_params.read_access = SEC_OPEN;
    add_descr_params.p_value = (uint8_t *)"Office Monitoring ";
    err_code = descriptor_add((p_cus->office_monitoring_char_handles).value_handle,
                              &add_descr_params,
                              &p_cus->office_monitoring_char_handles.user_desc_handle);
                              
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    return NRF_SUCCESS;
}


/**@brief Function for updating an office occupancy on the offices occupancy ble characteristic.
 *
 * @param[in]   p_cus             Custom service structure.
 * @param[in]   p_new_value       Buttons states.
 * @param[in]   conn_handle       Connection handle.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */

uint32_t ble_cus_office_occupancy_update(ble_cus_t * p_cus, uint8_t  * p_new_value, uint16_t length)
{

    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = length;
    gatts_value.offset  = 0;
    gatts_value.p_value = p_new_value;

    // Update database.
    return sd_ble_gatts_value_set(p_cus->conn_handle,
                                  p_cus->office_monitoring_char_handles.value_handle,
                                  &gatts_value);
}

/**@brief Function for notifiying and updating office monitoring ble characteristic value.
 *
 * @param[in]   p_cus             Custom service structure.
 * @param[in]   p_new_value       Buttons states.
 * @param[in]   length            size of p_new_value.
 * @param[in]   conn_handle       Connection handle.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_cus_char_update(ble_cus_t * p_cus, uint8_t  * p_value, uint16_t length, uint16_t conn_handle)
{
    
  uint32_t err_code;
  err_code = ble_cus_office_occupancy_update(p_cus, p_value, sizeof(p_value));
  if (err_code != NRF_SUCCESS)
  {
      NRF_LOG_INFO("update failure");  
    return err_code;
  }
    
        // Send value if connected and notifying.
    if ((p_cus->conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t params;
        //uint16_t len = sizeof(p_value);
        memset(&params, 0, sizeof(params));
        params.type   = BLE_GATT_HVX_NOTIFICATION;
        params.handle = p_cus->office_monitoring_char_handles.value_handle;
        params.p_data = p_value;
        params.p_len  = &length;
        
        err_code =  sd_ble_gatts_hvx(p_cus->conn_handle, &params);
        //NRF_LOG_INFO("notif success %x %d", err_code, err_code);
    }
    else
    {
      NRF_LOG_INFO("notif failure");  
      err_code = NRF_ERROR_INVALID_STATE;
    }
    
    return err_code;
}

/**@brief Function for updating the flash.
 */
void update_memory(void)
{
    if (action != Monitor_Office)
    {
        office_item read_table[OFFICE_COUNT];
        read_office_table_from_flash(read_table);
        if (does_office_exist(read_table, words[1]))
        {
            if (action == Clear_Office)
            {
                clear_office(read_table, words[1]);
            }
            else if (action == Reserve_Office)
            {
                reserve_office(read_table, words[1], words[2]);
            }
            erase_office_table_from_flash();
            write_office_table_to_flash(read_table);
        }
    }        
}


