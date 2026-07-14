/**************************************************************************//**
 * @file     mem_alloc.c
 * @version  V1.00
 * @brief    USB Host library memory allocation functions.
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2021 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NuMicro.h"

#include "usb.h"

/// @cond HIDDEN_SYMBOLS
#ifdef MEM_DEBUG
#undef MEM_DEBUG
#endif

#define NEW_CODE_ADDED
//#define MEM_DEBUG

#ifdef MEM_DEBUG
#define mem_debug       printf
#else
#define mem_debug(...)
#endif

#ifdef __ICCARM__
#pragma data_alignment=32
static uint8_t  _mem_pool[MEM_POOL_UNIT_NUM][MEM_POOL_UNIT_SIZE];
#else
static uint8_t _mem_pool[MEM_POOL_UNIT_NUM][MEM_POOL_UNIT_SIZE] __attribute__((aligned(32)));
#endif
static uint8_t  _unit_used[MEM_POOL_UNIT_NUM];

static volatile int  _usbh_mem_used;
static volatile int  _usbh_max_mem_used;
static volatile int  _mem_pool_used;

UDEV_T * g_udev_list;

uint8_t  _dev_addr_pool[128];
static volatile int  _device_addr;

static  int  _sidx = 0;;

/*--------------------------------------------------------------------------*/
/*   Memory alloc/free recording                                            */
/*--------------------------------------------------------------------------*/

void usbh_memory_init(void)
{
    if(sizeof(TD_T) > MEM_POOL_UNIT_SIZE)
    {
        // USB_DBG_LOCK();
		USB_error("%s", "TD_T - MEM_POOL_UNIT_SIZE too small!\r\n");
		// USB_DBG_UNLOCK();
        while(1);
    }

    if(sizeof(ED_T) > MEM_POOL_UNIT_SIZE)
    {
        // USB_DBG_LOCK();
		USB_error("%s", "ED_T - MEM_POOL_UNIT_SIZE too small!\r\n");
		// USB_DBG_UNLOCK();
        while(1);
    }

    _usbh_mem_used = 0L;
    _usbh_max_mem_used = 0L;

    memset(_unit_used, 0, sizeof(_unit_used));
    _mem_pool_used = 0;
    _sidx = 0;

    g_udev_list = NULL;

    memset(_dev_addr_pool, 0, sizeof(_dev_addr_pool));
    _device_addr = 1;
}

uint32_t  usbh_memory_used(void)
{
    printf("USB static memory: %d/%d, heap used: %d\n", _mem_pool_used, MEM_POOL_UNIT_NUM, _usbh_mem_used);
    return _usbh_mem_used;
}

static void  memory_counter(int size)
{
    _usbh_mem_used += size;
    if(_usbh_mem_used > _usbh_max_mem_used)
        _usbh_max_mem_used = _usbh_mem_used;
}

void * usbh_alloc_mem(int size)
{
    void  *p;

    // p = malloc(size);
    p = SENS_MEM_ZALLOC(size);
    if(p == NULL)
    {
        // USB_DBG_LOCK();
        USB_error("usbh_alloc_mem failed! %d\r\n", size);
        // USB_DBG_UNLOCK();
        return NULL;
    }

    //memset(p, 0, size);	//ZALLOC already clear memory to 0
    memory_counter(size);
    return p;
}

void usbh_free_mem(void *p, int size)
{
    // free(p);
    SENS_MEM_FREE(p);
    memory_counter(0 - size);
}

/*--------------------------------------------------------------------------*/
/*   USB device allocate/free                                               */
/*--------------------------------------------------------------------------*/

UDEV_T * alloc_device(void)
{
    UDEV_T  *udev;

    //udev = malloc(sizeof(*udev));
    udev = SENS_MEM_ZALLOC(sizeof(*udev));
    if(udev == NULL)
    {
        // USB_DBG_LOCK();
        USB_error("%s", "alloc_device failed!\r\n");
        // USB_DBG_UNLOCK();
        return NULL;
    }
    //memset(udev, 0, sizeof(*udev));	//ZALLOC already clear memory to 0
    memory_counter(sizeof(*udev));
    udev->cur_conf = -1;                    /* must! used to identify the first SET CONFIGURATION */
    udev->next = g_udev_list;               /* chain to global device list */
    g_udev_list = udev;
    return udev;
}

void free_device(UDEV_T *udev)
{
    UDEV_T  *d;

    if(udev == NULL)
        return;

    if(udev->cfd_buff != NULL)
        usbh_free_mem(udev->cfd_buff, MAX_DESC_BUFF_SIZE);

    /*
     *  Remove it from the global device list
     */
    if(g_udev_list == udev)
    {
        g_udev_list = g_udev_list->next;
    }
    else
    {
        d = g_udev_list;
        while(d != NULL)
        {
            if(d->next == udev)
            {
                d->next = udev->next;
                break;
            }
            d = d->next;
        }
    }
    //free(udev);
	SENS_MEM_FREE(udev);
    memory_counter(-sizeof(*udev));
}

int  alloc_dev_address(void)
{
    _device_addr++;

    if(_device_addr >= 128)
        _device_addr = 1;

    while(1)
    {
        if(_dev_addr_pool[_device_addr] == 0)
        {
            _dev_addr_pool[_device_addr] = 1;
            return _device_addr;
        }
        _device_addr++;
        if(_device_addr >= 128)
            _device_addr = 1;
    }
}

void  free_dev_address(int dev_addr)
{
    if(dev_addr < 128)
        _dev_addr_pool[dev_addr] = 0;
}

/*--------------------------------------------------------------------------*/
/*   UTR (USB Transfer Request) allocate/free                               */
/*--------------------------------------------------------------------------*/

UTR_T * alloc_utr(UDEV_T *udev)
{
    UTR_T  *utr;

    //utr = malloc(sizeof(*utr));
	utr = SENS_MEM_ZALLOC(sizeof(*utr));
    if(utr == NULL)
    {
        // USB_DBG_LOCK();
		USB_error("%s", "alloc_utr failed!\r\n");
		// USB_DBG_UNLOCK();
        return NULL;
    }
    memory_counter(sizeof(*utr));
    //memset(utr, 0, sizeof(*utr));	//ZALLOC already clear memory to 0
    utr->udev = udev;
    mem_debug("[ALLOC] [UTR] - 0x%x\n", (int)utr);
    return utr;
}

void free_utr(UTR_T *utr)
{
    if(utr == NULL)
        return;

    mem_debug("[FREE] [UTR] - 0x%x\n", (int)utr);
    //free(utr);
	SENS_MEM_FREE(utr);
    memory_counter(0 - (int)sizeof(*utr));
}

/*--------------------------------------------------------------------------*/
/*   OHCI ED allocate/free                                                  */
/*--------------------------------------------------------------------------*/

ED_T * alloc_ohci_ED(void)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int    i;
    ED_T   *ed;
#ifdef NEW_CODE_ADDED
    __disable_irq();
#endif
    for(i = 0; i < MEM_POOL_UNIT_NUM; i++)
    {
        if(_unit_used[i] == 0)
        {
            _unit_used[i] = 1;
            _mem_pool_used++;
#ifdef NEW_CODE_ADDED
            __set_PRIMASK(irq_state);
#endif
            ed = (ED_T *)&_mem_pool[i];
            memset(ed, 0, sizeof(*ed));
			//USB_DBG_LOCK();
            mem_debug("[ALLOC] [ED] - 0x%x\r\n", (int)ed);
			//USB_DBG_UNLOCK();
            return ed;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
	mem_debug("%s", "alloc_ohci_ED failed!\r\n");
    return NULL;
}

void free_ohci_ED(ED_T *ed)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int      i;
#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = 0; i < MEM_POOL_UNIT_NUM; i++)
    {
        if((uint32_t)&_mem_pool[i] == (uint32_t)ed)
        {
            mem_debug("[FREE]  [ED] - 0x%x\n", (int)ed);
            _unit_used[i] = 0;
            _mem_pool_used--;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            return;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
	mem_debug("%s", "free_ohci_ED - not found! (ignored in case of multiple UTR)\r\n");
}

/*--------------------------------------------------------------------------*/
/*   OHCI TD allocate/free                                                  */
/*--------------------------------------------------------------------------*/
TD_T * alloc_ohci_TD(UTR_T *utr)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int    i;
    TD_T   *td;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = 0; i < MEM_POOL_UNIT_NUM; i++)
    {
        if(_unit_used[i] == 0)
        {
            _unit_used[i] = 1;
            _mem_pool_used++;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            td = (TD_T *)&_mem_pool[i];

            memset(td, 0, sizeof(*td));
            td->utr = utr;
			mem_debug("[ALLOC] [TD] - 0x%x\r\n", (int)td);
            return td;
        }
    }

#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
	mem_debug("%s", "alloc_ohci_TD failed!\r\n");
    return NULL;
}

void free_ohci_TD(TD_T *td)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int   i;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = 0; i < MEM_POOL_UNIT_NUM; i++)
    {
        if((uint32_t)&_mem_pool[i] == (uint32_t)td)
        {
            mem_debug("[FREE]  [TD] - 0x%x\n", (int)td);
            _unit_used[i] = 0;
            _mem_pool_used--;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            return;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
    mem_debug("%s", "free_ohci_TD - not found!\r\n");
}

/*--------------------------------------------------------------------------*/
/*   EHCI QH allocate/free                                                  */
/*--------------------------------------------------------------------------*/
QH_T * alloc_ehci_QH(void)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int    i;
    QH_T   *qh = NULL;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = (_sidx + 1) % MEM_POOL_UNIT_NUM; i != _sidx; i = (i + 1) % MEM_POOL_UNIT_NUM)
    {
        if(_unit_used[i] == 0)
        {
            _unit_used[i] = 1;
            _sidx = i;
            _mem_pool_used++;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            qh = (QH_T *)&_mem_pool[i];
            memset(qh, 0, sizeof(*qh));
            mem_debug("[ALLOC] [QH] - 0x%x\n", (int)qh);
            break;
        }
    }
    if(qh == NULL)
    {
#ifdef NEW_CODE_ADDED
		__set_PRIMASK(irq_state);
#endif
        mem_debug("%s", "alloc_ehci_QH failed!\r\n");
        return NULL;
    }
    qh->Curr_qTD        = QTD_LIST_END;
    qh->OL_Next_qTD     = QTD_LIST_END;
    qh->OL_Alt_Next_qTD = QTD_LIST_END;
    qh->OL_Token        = QTD_STS_HALT;
    return qh;
}

void free_ehci_QH(QH_T *qh)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int      i;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = 0; i < MEM_POOL_UNIT_NUM; i++)
    {
        if((uint32_t)&_mem_pool[i] == (uint32_t)qh)
        {
            mem_debug("[FREE]  [QH] - 0x%x\n", (int)qh);
            _unit_used[i] = 0;
            _mem_pool_used--;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            return;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
	mem_debug("%s", "free_ehci_QH - not found! (ignored in case of multiple UTR)\r\n");
}

/*--------------------------------------------------------------------------*/
/*   EHCI qTD allocate/free                                                 */
/*--------------------------------------------------------------------------*/
qTD_T * alloc_ehci_qTD(UTR_T *utr)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int     i;
    qTD_T   *qtd;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = (_sidx + 1) % MEM_POOL_UNIT_NUM; i != _sidx; i = (i + 1) % MEM_POOL_UNIT_NUM)
    {
        if(_unit_used[i] == 0)
        {
            _unit_used[i] = 1;
            _sidx = i;
            _mem_pool_used++;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            qtd = (qTD_T *)&_mem_pool[i];

            memset(qtd, 0, sizeof(*qtd));
            qtd->Next_qTD     = QTD_LIST_END;
            qtd->Alt_Next_qTD = QTD_LIST_END;
            qtd->Token        = 0x1197B3F; // QTD_STS_HALT;  visit_qtd() will not remove a qTD with this mark. It means the qTD still not ready for transfer.
            qtd->utr = utr;
            mem_debug("[ALLOC] [qTD] - 0x%x\n", (int)qtd);
            return qtd;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
    mem_debug("%s", "alloc_ehci_qTD failed!\r\n");
    return NULL;
}

void free_ehci_qTD(qTD_T *qtd)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int   i;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = 0; i < MEM_POOL_UNIT_NUM; i++)
    {
        if((uint32_t)&_mem_pool[i] == (uint32_t)qtd)
        {
            mem_debug("[FREE]  [qTD] - 0x%x\r\n", (int)qtd);
            _unit_used[i] = 0;
            _mem_pool_used--;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            return;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
    mem_debug("free_ehci_qTD 0x%x - not found!\r\n", (int)qtd);
}

/*--------------------------------------------------------------------------*/
/*   EHCI iTD allocate/free                                                 */
/*--------------------------------------------------------------------------*/
iTD_T * alloc_ehci_iTD(void)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int     i;
    iTD_T   *itd;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = (_sidx + 1) % MEM_POOL_UNIT_NUM; i != _sidx; i = (i + 1) % MEM_POOL_UNIT_NUM)
    {
        if(i + 2 >= MEM_POOL_UNIT_NUM)
            continue;

        if((_unit_used[i] == 0) && (_unit_used[i + 1] == 0))
        {
            _unit_used[i] = _unit_used[i + 1] = 1;
            _sidx = i + 1;
            _mem_pool_used += 2;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            itd = (iTD_T *)&_mem_pool[i];
            memset(itd, 0, sizeof(*itd));
			mem_debug("[ALLOC] [iTD] - 0x%x\r\n", (int)itd);
            return itd;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
	mem_debug("%s", "alloc_ehci_iTD failed!\r\n");
    return NULL;
}

void free_ehci_iTD(iTD_T *itd)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int   i;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = 0; i + 1 < MEM_POOL_UNIT_NUM; i++)
    {
        if((uint32_t)&_mem_pool[i] == (uint32_t)itd)
        {
			mem_debug("[FREE]  [iTD] - 0x%x\r\n", (int)itd);
            _unit_used[i] = _unit_used[i + 1] = 0;
            _mem_pool_used -= 2;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            return;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
	mem_debug("free_ehci_iTD 0x%x - not found!\r\n", (int)itd);
}

/*--------------------------------------------------------------------------*/
/*   EHCI iTD allocate/free                                                 */
/*--------------------------------------------------------------------------*/
siTD_T * alloc_ehci_siTD(void)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int     i;
    siTD_T  *sitd;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = (_sidx + 1) % MEM_POOL_UNIT_NUM; i != _sidx; i = (i + 1) % MEM_POOL_UNIT_NUM)
    {
        if(_unit_used[i] == 0)
        {
            _unit_used[i] = 1;
            _sidx = i;
            _mem_pool_used ++;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            sitd = (siTD_T *)&_mem_pool[i];
            memset(sitd, 0, sizeof(*sitd));
            mem_debug("[ALLOC] [siTD] - 0x%x\r\n", (int)sitd);
            return sitd;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
	mem_debug("%s", "alloc_ehci_siTD failed!\r\n");
    return NULL;
}

void free_ehci_siTD(siTD_T *sitd)
{
#ifdef NEW_CODE_ADDED
	uint32_t irq_state = __get_PRIMASK();
#endif
    int   i;

#ifdef NEW_CODE_ADDED
	__disable_irq();
#endif
    for(i = 0; i < MEM_POOL_UNIT_NUM; i++)
    {
        if((uint32_t)&_mem_pool[i] == (uint32_t)sitd)
        {
            mem_debug("[FREE]  [siTD] - 0x%x\r\n", (int)sitd);
			_unit_used[i] = 0;
            _mem_pool_used--;
#ifdef NEW_CODE_ADDED
			__set_PRIMASK(irq_state);
#endif
            return;
        }
    }
#ifdef NEW_CODE_ADDED
	__set_PRIMASK(irq_state);
#endif
	mem_debug("free_ehci_siTD 0x%x - not found!\r\n", (int)sitd);
}

/// @endcond HIDDEN_SYMBOLS
