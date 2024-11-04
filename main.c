/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the MAC Provisioning Example
*              for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2021-2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/


/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include <ctype.h>
#include "cy_wcm.h"
#include "whd_wifi_api.h"

/* RTOS header file */
#include "cyabs_rtos.h"


/*******************************************************************************
 * Macros
 ********************************************************************************/
#define MAX_WHD_INTERFACE                   (2)


/*******************************************************************************
 * Global Variables
 ********************************************************************************/
/* Queue handler */
extern whd_interface_t whd_ifs[MAX_WHD_INTERFACE];

/* Stores a MAC address (17 characters with colons) */
char macAddress[18] = {0};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void readmac();

static int writemac();


/*******************************************************************************
 * Function Name: main
 ********************************************************************************
 * Summary:
 *  System entrance point. This function sets up user tasks and then starts
 *  the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 *******************************************************************************/
int main()
{
    cy_rslt_t result;

    /* Initialize the board support package. */
    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    cyhal_syspm_lock_deepsleep();

    /* Enable global interrupts. */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port. */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("\x1b[2J\x1b[;H");
    printf("****************** "
           "MAC Provisioning "
           "****************** \n");

    cy_wcm_config_t config = {.interface = CY_WCM_INTERFACE_TYPE_AP_STA};
    result = cy_wcm_init(&config);
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Wi-Fi Connection Manager initialization failed!\n");
    }

    for(;;)
    {
        printf("\n******************** MENU **********************\r\n");
        printf("**1) Press '1' to write the MAC address       **\r\n");
        printf("**2) Press '2' to read the MAC address        **\r\n");
        printf("************************************************\r\n\n");

        uint8_t readbyte;
        while(!cyhal_uart_readable(&cy_retarget_io_uart_obj))
        {
            cy_rtos_delay_milliseconds(100);
        }
        cyhal_uart_getc(&cy_retarget_io_uart_obj , &readbyte, 0);
        switch (readbyte)
        {
        case '1':
            writemac();
            memset(macAddress, 0, sizeof(macAddress));
            break;
        case '2':
            readmac();
            break;
        default:
            printf("Invalid Input\r\n\n");
            break;
        }
    }
}

/******************************************************************************
 * Function Name: readMac
 *******************************************************************************
 * Summary:
 *  Reads the MAC address
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void readmac()
{
    cy_rslt_t result;
    uint8_t mac1[6] = {0};

    result = whd_wifi_get_mac_address(whd_ifs[CY_WCM_INTERFACE_TYPE_STA], (whd_mac_t *)&mac1);
    if(result == CY_RSLT_SUCCESS)
    {
        printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac1[0],mac1[1],mac1[2],mac1[3],mac1[4],mac1[5]);
    }
}

/******************************************************************************
 * Function Name: uart_read
 *******************************************************************************
 * Summary:
 *  reads input from UART
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void uart_read() {
    uint8_t macbyte;
    uint32_t macbytedisp;

    printf("Enter the MAC Address in this format: XX:XX:XX:XX:XX:XX\n");
    for (;;) {
        while (!cyhal_uart_readable(&cy_retarget_io_uart_obj)) {
            cy_rtos_delay_milliseconds(100);
        }
        cyhal_uart_getc(&cy_retarget_io_uart_obj, &macbyte, 0);
        macbytedisp = (char)macbyte;
        cyhal_uart_putc(&cy_retarget_io_uart_obj, macbytedisp);

        if (strlen(macAddress) < 17)
        {
            macAddress[strlen(macAddress)] = (char)macbyte;
            macAddress[strlen(macAddress) + 1] = '\0';
        }

        if (strlen(macAddress) == 2 && isxdigit(macbyte)) 
        {
            int hexValue = (macbyte >= '0' && macbyte <= '9') ? (macbyte - '0') :
                           (macbyte >= 'A' && macbyte <= 'F') ? (macbyte - 'A' + 10) :
                           (macbyte - 'a' + 10);
            if ((hexValue & 1) == 1) 
            {
                printf("\n");
                printf("Error: Invalid MAC Address.\n");
                printf("Enter the MAC Address again in valid format:\n");
                memset(macAddress, 0, sizeof(macAddress));
                continue;
            }
        }

        if ((macbyte == ':' && (strlen(macAddress) % 3) != 0) || (((strlen(macAddress) % 3) == 0) && (macbyte != ':')))
        {
            printf("\n");
            printf("Error: Invalid MAC Address.\n");
            printf("Enter the MAC Address again in valid format:\n");
            memset(macAddress, 0, sizeof(macAddress));
            continue;
        }

        // Check if the MAC address is complete
        if (strlen(macAddress) == 17)
        {
            break;
        }
    }
}

/******************************************************************************
 * Function Name: writemac
 *******************************************************************************
 * Summary:
 *  Parse the user provided data and send it to the FW in tuple format
 *
 * Return:
 *  int
 *
 ******************************************************************************/
int writemac()
{
    cy_rslt_t result;
    char *data, *tagval;
    char var[14], bytes[6], hexstr[3];
    uint8 tag, subtag, taglen, region, tagvallen = 0;
    uint32 datalen = 0, len = 0;
    int j = 0;
    /* Stores a MAC address (12 characters without colons) */
    char truncatedMac[13];

    /* region tag and subtag are used to identify the location in the OTP
     * to program the MAC address */
    region = 7;
    tag = 128;
    subtag = 25;

    /* Represents the size of the data to be stored in the location pointed
     * by the combination of above variables, MAC address in this case */
    taglen = 6;

    memset(var, 0, sizeof(var));
    uart_read();
    data = macAddress;
    printf("\n");
    printf("Writing mac %s to non-secure otp......\n", data);
    memset(bytes, 0, sizeof(bytes));

    /* Extracting the numeric hex characters into truncatedMac */
    for (int i = 0; i < strlen(data); i++)
    {
        if (data[i] != ':')
        {
            truncatedMac[j] = data[i];
            j++;
        }
    }

    truncatedMac[j] = '\0';
    tagval = truncatedMac;
    tagvallen = taglen;

    /* Adding the length for subtag */
    taglen++;

    /* Convert and store hex byte values */
    for (int i = 0; i < 6; i++)
    {
        memset(hexstr, 0, sizeof(hexstr));
        hexstr[0] = *tagval;
        hexstr[1] = *(tagval + 1);
        if (!isxdigit((int)hexstr[0]) || !isxdigit((int)hexstr[1]))
        {
            printf( "invalid hex digit(s) in %c%c\n",hexstr[0], hexstr[1]);
            goto done;
        }
        hexstr[2] = '\0';
        bytes[i] = (char)strtol(hexstr, NULL, 16);
        tagval += 2;
    }

    /* Convert the user provided data into tuple format and send it to the FW */
    len = sizeof(region) + sizeof(tag) + sizeof(taglen) + sizeof(subtag) + tagvallen;
    memcpy(var, &len, sizeof(len));
    datalen = sizeof(len);
    memcpy(&var[datalen], &region, sizeof(region));
    datalen += sizeof(region);
    memcpy(&var[datalen], &tag, sizeof(tag));
    datalen += sizeof(tag);
    memcpy(&var[datalen], &taglen, sizeof(taglen));
    datalen += sizeof(taglen);
    memcpy(&var[datalen], &subtag, sizeof(subtag));
    datalen += sizeof(subtag);
    memcpy(&var[datalen], bytes, tagvallen);
    datalen += tagvallen;
    len = datalen;
    result = whd_wifi_set_mac_addr_via_otp(whd_ifs[CY_WCM_INTERFACE_TYPE_STA], var, len);
    if(result == CY_RSLT_SUCCESS)
    {
        printf("Successfully written the MAC\n");
    }

done:
    return 0;
}

/* [] END OF FILE */
