#ifndef _CAN_CONFIG_H
#define _CAN_CONFIG_H

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <e> CAN1 [Driver_CAN1]
// <i> Configuration settings for Driver_CAN1 in component ::Drivers:CAN
#define RTE_CAN1                      	1

//   <o> CAN1_RX Pin <0=>PB8 <1=>PD0
#define RTE_CAN1_TX_ID                  0
#if    (RTE_CAN1_TX_ID == 0)
#define RTE_CAN1_RX_PORT                GPIOB
#define RTE_CAN1_RX_BIT                 8
#elif  (RTE_CAN1_RX_ID == 1)
#define RTE_CAN1_RX_PORT                GPIOD
#define RTE_CAN1_RX_BIT                 0
#else
#error "Invalid CAN1_RX Pin Configuration!"
#endif

//   <o> CAN1_TX Pin <0=>PB9 <1=>PD1
#define RTE_CAN1_TX_ID 	                0
#if    (RTE_CAN1_TX_ID == 0)
#define RTE_CAN1_TX_PORT                GPIOB
#define RTE_CAN1_TX_BIT                 9
#elif  (RTE_CAN1_TX_ID == 1)
#define RTE_CAN1_TX_PORT                GPIOD
#define RTE_CAN1_TX_BIT                 1
#else
#error "Invalid CAN1_TX Pin Configuration!"
#endif

// </e>

// <e> CAN2 [Driver_CAN2]
// <i> Configuration settings for Driver_CAN2 in component ::Drivers:CAN
#define RTE_CAN2                      	0

//   <o> CAN2_RX Pin <0=>PB5 <1=>PB12
#define RTE_CAN2_TX_ID                  0
#if    (RTE_CAN2_TX_ID == 0)
#define RTE_CAN2_RX_PORT                GPIOB
#define RTE_CAN2_RX_BIT                 5
#elif  (RTE_CAN2_RX_ID == 1)
#define RTE_CAN2_RX_PORT                GPIOB
#define RTE_CAN2_RX_BIT                 12
#else
#error "Invalid CAN2_RX Pin Configuration!"
#endif

//   <o> CAN2_TX Pin <0=>PB6 <1=>PB13
#define RTE_CAN2_TX_ID 	                0
#if    (RTE_CAN2_TX_ID == 0)
#define RTE_CAN2_TX_PORT                GPIOB
#define RTE_CAN2_TX_BIT                 6
#elif  (RTE_CAN2_TX_ID == 1)
#define RTE_CAN2_TX_PORT                GPIOB
#define RTE_CAN2_TX_BIT                 13
#else
#error "Invalid CAN2_TX Pin Configuration!"
#endif

// </e>

// <o0> Number of transmit objects for CAN <1-1024>
//      <i> Determines the size of software message buffer for transmitting.
//      <i> Default: 20
// <o1> Number of receive objects for CAN <1-1024>
//      <i> Determines the size of software message buffer for receiving.
//      <i> Default: 20
#define CAN_No_SendObjects     32
#define CAN_No_ReceiveObjects  32

#endif
