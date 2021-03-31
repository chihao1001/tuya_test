/**********************************Copyright (c)**********************************
**                     All rights reserved (C), 2015-2020, Tuya
**
**                             http://www.tuya.com
**
*********************************************************************************/
/**
 * @file    sketch_nov12a.ino
 * @author  Tuya Team
 * @brief   Arduino-based wifi-color light demo
 */
#include "wifi.h"
#include <SoftwareSerial.h>
#include "tuya_type.h"

SoftwareSerial my_serial(0, 1); // RX, TX
#define RELAY 10
#define WIFI_STATE_LED PA5     //wifi indicator pin
#define WIFI_STATE_KEY PC13    //Wifi status button

i32 g_time_cnt = 0, g_cnt = 0, g_init_flag = 0;

/**
  * @brief Initialization
  * @param None
  * @retval None
  */
void setup(void) 
{
    pinMode(RELAY, OUTPUT);   //Relay IO initialization
    digitalWrite(RELAY, LOW);

    pinMode(WIFI_STATE_KEY, INPUT);     //Button detection initialization
    pinMode(WIFI_STATE_LED, OUTPUT);    //WiFi status indicator initialization

    my_serial.begin(9600);     //Software serial port initialization
    my_serial.println("my_serial init successful!");
    Serial.begin(115200);     //PA3 RX   PA2 TX
    Serial.println("serial init successful!");

    wifi_protocol_init();
}

/**
  * @brief Loop provided by arduino
  * @param None
  * @retval None
  */
void loop(void) 
{
    wifi_uart_service();
    my_serial_event();      //Serial port receive processing
    key_scan();             //Reset distribution network button detection
    if (g_init_flag == 0) {
        g_time_cnt++;
        if (g_time_cnt % 6000 == 0) {   //Polling settings
            g_time_cnt = 0;
            g_cnt ++;
        }
        wifi_stat_led(&g_cnt);   //WIFI status processing
    }
}

/**
  * @brief mcu serial port to receive information
  * @param None
  * @retval None
  */
void my_serial_event(void) {
    if (my_serial.available()) {
        u8 ch = (u8)my_serial.read();
        uart_receive_input(ch);
    }
}

/**
  * @brief Button polling
  * @param None
  * @retval None
  */
void key_scan(void)
{
    STATIC i8 ap_ez_change = 0;
    u8 buttonState  = HIGH;
    buttonState = digitalRead(WIFI_STATE_KEY);  //the state of key
    if (buttonState == LOW) {
        delay(3000);
        buttonState = digitalRead(WIFI_STATE_KEY);
        if (buttonState == LOW) {
            g_init_flag = 0;
            switch (ap_ez_change) {
                case 0 :
                    mcu_set_wifi_mode(SMART_CONFIG);
                    printf("the current mode is SMART_CONFIG\r\n");
                    break;
                case 1 :
                    mcu_set_wifi_mode(AP_CONFIG);
                    printf("the current mode is AP_CONFIG\r\n");
                    break;
                default:
                    break;
            }
            ap_ez_change = !ap_ez_change;
            while (digitalRead(WIFI_STATE_KEY) == LOW);   //Prevent the mode from being switched after long pressing the button
        }   
    }
}

/**
  * @brief Set the corresponding LED flicker according to the wifi status(Blinking time is controlled by polling,
so the flashing time may be affected by functions such as printf)
  * @param[in] {g_cnt} Count value
  * @param None
  * @retval None
  */
void wifi_stat_led(i32 *g_cnt)
{
    switch (mcu_get_wifi_work_state()) {
      case SMART_CONFIG_STATE:  //0x00
          g_init_flag = 0;
          if (*g_cnt == 2) {
              *g_cnt = 0;
          }
          if (*g_cnt % 2 == 0) {    //LED flash quckily 
              digitalWrite(WIFI_STATE_LED, HIGH);
          } else {
              digitalWrite(WIFI_STATE_LED, LOW);
          }
          break;
      case AP_STATE:  //0x01
          g_init_flag = 0;
          if (*g_cnt >= 15) {
              *g_cnt = 0;
          }
          if (*g_cnt  == 0) {   //LED flash slowly      
              digitalWrite(WIFI_STATE_LED, LOW);
          } else if (*g_cnt == 7) {
              digitalWrite(WIFI_STATE_LED, HIGH);
          } else {
              ;
          }
          break;
      case WIFI_NOT_CONNECTED:  //0x02
          break;
      case WIFI_CONNECTED:  //0x03
          break;
      case WIFI_CONN_CLOUD:  //0x04
          if ( 0 == g_init_flag ) {
              digitalWrite(WIFI_STATE_LED, HIGH);   //LED Always on
              g_init_flag = 1;                  //The light can be controlled after wifi is connected
              *g_cnt = 0;
          }
          break;
      default:
          digitalWrite(WIFI_STATE_LED, HIGH);   
          break;
    }
}
