/**************************************************************************//**
 * @file     m460_emac.h
 * @version  V3.00
 * @brief    M460 EMAC driver header file
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef  __M460_EMAC_H__
#define  __M460_EMAC_H__

//#include "NuMicro.h"
#include "sensminiCfg.h"
#include "synopGMAC_network_interface.h"
#include "lwip/netif.h"


#define THREAD_STACKSIZE	512

void EMAC_Open(uint8_t *macaddr, struct netif *netif);
uint32_t EMAC_ReceivePkt(void);
int32_t  EMAC_TransmitPkt(uint8_t *pbuf, uint32_t len);
uint8_t* EMAC_AllocatePktBuf(void);


enum ethernet_link_status {
	ETH_LINK_UNDEFINED = 0,
	ETH_LINK_UP,
	ETH_LINK_DOWN,
	ETH_LINK_NEGOTIATING
};

#endif  /* __M460_EMAC_H__ */
