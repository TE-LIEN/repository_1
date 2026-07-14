/*
 * Copyright (c) 2015-2016, Nuvoton Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "m2354_mbedtls_config.h"
#include "cryptoMisc.h"
//#include "cmsis.h"
//#include "mbed_assert.h"
//#include "mbed_atomic.h"
//#include "mbed_critical.h"
//#include "mbed_error.h"
#include <limits.h>
//#include "nu_modutil.h"
#include "nu_bitutil.h"
#include "sensSystem.h"
//#include "platform/SingletonPtr.h"
//#include "platform/PlatformMutex.h"

/* Consideration for choosing proper synchronization mechanism
 *
 * 1. We choose mutex to synchronize access to crypto non-SHA AC. We can guarantee:
 *    (1) No deadlock
 *        We just lock mutex for a short sequence of operations rather than the whole lifetime
 *        of crypto context.
 *    (2) No priority inversion
 *        Mutex supports priority inheritance and it is enabled.
 * 2. We choose atomic flag to synchronize access to crypto SHA AC. We can guarantee:
 *    (1) No deadlock
 *        With SHA AC not supporting context save & restore, we provide SHA S/W fallback when
 *        SHA AC is not available.
 *    (2) No biting CPU
 *        Same reason as above.
 */

#ifdef CRYPTO_USE_M460_LIB
static SENS_SEM_STRUCT crypto_prng_mutex=NULL;
#endif

/* Mutex for crypto AES AC management */
static SENS_SEM_STRUCT crypto_aes_mutex=NULL;

/* Mutex for crypto DES AC management */
#ifdef MBEDTLS_DES_ALT
static SENS_SEM_STRUCT crypto_des_mutex = NULL;
#endif
/* Mutex for crypto ECC AC management */
static SENS_SEM_STRUCT crypto_ecc_mutex=NULL;

/* Atomic flag for crypto SHA AC management */
//static core_util_atomic_flag crypto_sha_atomic_flag = CORE_UTIL_ATOMIC_FLAG_INIT;
static volatile uint8_t crypto_sha_atomic_flag = 0;

#ifdef MBEDTLS_RSA_ALT
static SENS_SEM_STRUCT crypto_rsa_mutex=NULL;
#endif

static char *privateKeyD = NULL;
static volatile uint8_t privateKeyCreated = 0;
/* Crypto (AES, DES, SHA, etc.) init counter. Crypto's keeps active as it is non-zero. */
static uint16_t crypto_init_counter = 0U;

/* Crypto done flags */
#define CRYPTO_DONE_OK              BIT0    /* Done with OK */
#define CRYPTO_DONE_ERR             BIT1    /* Done with error */
#define NU_CRYPTO_DONE_OK						BIT0
#define NU_CRYPTO_DONE_ERR					BIT1

/* Track if PRNG H/W operation is done */
static volatile uint16_t crypto_prng_done;
/* Track if AES H/W operation is done */
static volatile uint16_t crypto_aes_done;
/* Track if DES H/W operation is done */
#ifdef MBEDTLS_DES_ALT
static volatile uint16_t crypto_des_done;
#endif
/* Track if ECC H/W operation is done */
static volatile uint16_t crypto_ecc_done;

#ifdef MBEDTLS_RSA_ALT
static volatile uint16_t	crypto_rsa_done;
#endif

static void crypto_submodule_prestart(volatile uint16_t *submodule_done);
static bool crypto_submodule_wait(volatile uint16_t *submodule_done, int32_t timeout_us);

/* As crypto init counter changes from 0 to 1:
 *
 * 1. Enable crypto clock
 * 2. Enable crypto interrupt
 */
void crypto_init(void)
{	/*core_util_critical_section_enter();
	if(crypto_init_counter == USHRT_MAX) 
	{	core_util_critical_section_exit();
		error("Crypto clock enable counter would overflow (> USHRT_MAX)");
	}
	core_util_atomic_incr_u16(&crypto_init_counter, 1);*/
	crypto_init_counter++;
	if(crypto_init_counter == 1) 
	{	SYS_UnlockReg();    // Unlock protected register
		CLK_EnableModuleClock(CRPT_MODULE);
#ifdef CRYPTO_USE_M460_LIB
		SYS_ResetModule(CRPT_RST);
#endif
		//SYS_LockReg();      // Lock protected register
		NVIC_EnableIRQ(CRPT_IRQn);
#ifdef CRYPTO_USE_M460_LIB
		//use trng
		
#endif
	}
	//core_util_critical_section_exit();
}

/* As crypto init counter changes from 1 to 0:
 *
 * 1. Disable crypto interrupt 
 * 2. Disable crypto clock
 */
void crypto_uninit(void)
{	/*core_util_critical_section_enter();
	if(crypto_init_counter == 0) 
	{	core_util_critical_section_exit();
		error("Crypto clock enable counter would underflow (< 0)");
	}
	core_util_atomic_decr_u16(&crypto_init_counter, 1);*/
	crypto_init_counter--;
	if(crypto_init_counter == 0) 
	{	NVIC_DisableIRQ(CRPT_IRQn);
		SYS_UnlockReg();    // Unlock protected register
		CLK_DisableModuleClock(CRPT_MODULE);
		//SYS_LockReg();      // Lock protected register
	}
	//core_util_critical_section_exit();
}

/* Implementation that should never be optimized out by the compiler */
void crypto_zeroize(void *v, size_t n)
{	volatile unsigned char *p = (volatile unsigned char*) v;
	while(n--) 
	{	*p++ = 0;
	}
}

/* Implementation that should never be optimized out by the compiler */
void crypto_zeroize32(uint32_t *v, size_t n)
{	volatile uint32_t *p = (volatile uint32_t*) v;
	while(n--) 
	{	*p++ = 0;
	}
}

#ifdef CRYPTO_USE_M460_LIB
void crypto_prng_acquire(void)
{	if(crypto_prng_mutex == NULL)
	{	SENS_SEM_INIT(crypto_prng_mutex, 1);
	}
	SENS_SEM_LOCK(crypto_prng_mutex);
}
void crypto_prng_release(void)
{	SENS_SEM_UNLOCK(crypto_prng_mutex);
}
#endif

void crypto_aes_acquire(void)
{	/* Don't check return code of Mutex::lock(void)
	 *
	 * This function treats RTOS errors as fatal system errors, so it can only return osOK.
	 * Use of the return value is deprecated, as the return is expected to become void in
	 * the future.
	 */
	//crypto_aes_mutex->lock();
	if(crypto_aes_mutex == NULL)
	{	SENS_SEM_INIT(crypto_aes_mutex, 1);
	}
	SENS_SEM_LOCK(crypto_aes_mutex);
}

void crypto_aes_release(void)
{	//crypto_aes_mutex->unlock();
	SENS_SEM_UNLOCK(crypto_aes_mutex);
}

#ifdef MBEDTLS_DES_ALT
void crypto_des_acquire(void)
{	/* Don't check return code of Mutex::lock(void) */
	//crypto_des_mutex->lock();
	if(crypto_des_mutex == NULL)
	{	SENS_SEM_INIT(crypto_des_mutex, 1);
	}
	SENS_SEM_LOCK(crypto_des_mutex);
}

void crypto_des_release(void)
{	//crypto_des_mutex->unlock();
	SENS_SEM_UNLOCK(crypto_des_mutex);
}
#endif

void crypto_ecc_acquire(void)
{	/* Don't check return code of Mutex::lock(void) */
	//crypto_ecc_mutex->lock();
	if(crypto_ecc_mutex == NULL)
	{	SENS_SEM_INIT(crypto_ecc_mutex, 1);
	}
	SENS_SEM_LOCK(crypto_ecc_mutex);
}

void crypto_ecc_release(void)
{	//crypto_ecc_mutex->unlock();
	SENS_SEM_UNLOCK(crypto_ecc_mutex);
}

#ifdef MBEDTLS_RSA_ALT
void crypto_rsa_acquire(void)
{	if(crypto_rsa_mutex == NULL)
	{	SENS_SEM_INIT(crypto_rsa_mutex, 1);
	}
	SENS_SEM_LOCK(crypto_rsa_mutex);
}

void crypto_rsa_release(void)
{	//crypto_ecc_mutex->unlock();
	SENS_SEM_UNLOCK(crypto_rsa_mutex);
}

#endif

uint8_t core_util_atomic_flag_test_and_set(volatile uint8_t *flagPtr)
{	taskENTER_CRITICAL();
	uint8_t currentValue = *flagPtr;
	*flagPtr = 1;
	taskEXIT_CRITICAL();
	return currentValue;
}

void core_util_atomic_flag_clear(volatile uint8_t *flagPtr)
{	taskENTER_CRITICAL();
	*flagPtr = 0;
	taskEXIT_CRITICAL();
}


bool crypto_sha_try_acquire(void)
{	return !core_util_atomic_flag_test_and_set(&crypto_sha_atomic_flag);
	//return 1;
}

void crypto_sha_release(void)
{	core_util_atomic_flag_clear(&crypto_sha_atomic_flag);
}

void crypto_prng_prestart(void)
{	crypto_submodule_prestart(&crypto_prng_done);
}

bool crypto_prng_wait2(int32_t timeout_us)
{	return crypto_submodule_wait(&crypto_prng_done, timeout_us);
}

bool crypto_prng_wait(void)
{	return crypto_prng_wait2(-1);
}

void crypto_aes_prestart(void)
{	crypto_submodule_prestart(&crypto_aes_done);
}

bool crypto_aes_wait2(int32_t timeout_us)
{	return crypto_submodule_wait(&crypto_aes_done, timeout_us);
}

bool crypto_aes_wait(void)
{	return crypto_aes_wait2(-1);
}

#ifdef MBEDTLS_DES_ALT
void crypto_des_prestart(void)
{	crypto_submodule_prestart(&crypto_des_done);
}

bool crypto_des_wait(void)
{	return crypto_submodule_wait(&crypto_des_done);
}
#endif
void crypto_ecc_prestart(void)
{	crypto_submodule_prestart(&crypto_ecc_done);
}

bool crypto_ecc_wait2(int32_t timeout_us)
{	return crypto_submodule_wait(&crypto_ecc_done, timeout_us);
}

bool crypto_ecc_wait(void)
{	return crypto_ecc_wait2(-1);
}

#ifdef MBEDTLS_RSA_ALT
void crypto_rsa_prestart(void)
{	crypto_submodule_prestart(&crypto_rsa_done);
}

bool crypto_rsa_wait(void)
{	return crypto_rsa_wait2(-1);
}

bool crypto_rsa_wait2(int32_t timeout_us)
{	return crypto_submodule_wait(&crypto_rsa_done, timeout_us);
}
#endif

bool crypto_dma_buff_compat(const void *buff, size_t buff_size, size_t size_aligned_to)
{	uint32_t buff_ = (uint32_t) buff;

	return (((buff_ & 0x03) == 0) &&                                        /* Word-aligned buffer base address */
        ((buff_size & (size_aligned_to - 1)) == 0) &&                       /* Crypto submodule dependent buffer size alignment */
        (((buff_ >> 28) == 0x2) && (buff_size <= (0x30000000 - buff_))));   /* 0x20000000-0x2FFFFFFF */
}

/* Overlap cases
 *
 * 1. in_buff in front of out_buff:
 *
 * in             in_end
 * |              |
 * ||||||||||||||||
 *     ||||||||||||||||
 *     |              |
 *     out            out_end
 *
 * 2. out_buff in front of in_buff:
 *
 *     in             in_end
 *     |              |
 *     ||||||||||||||||
 * ||||||||||||||||
 * |              |
 * out            out_end
 */
bool crypto_dma_buffs_overlap(const void *in_buff, size_t in_buff_size, const void *out_buff, size_t out_buff_size)
{	uint32_t in = (uint32_t) in_buff;
	uint32_t in_end = in + in_buff_size;
	uint32_t out = (uint32_t) out_buff;
	uint32_t out_end = out + out_buff_size;

	bool overlap = (in <= out && out < in_end) || (out <= in && in < out_end);
    
	return overlap;
}

static void crypto_submodule_prestart(volatile uint16_t *submodule_done)
{	*submodule_done = 0;
    
	/* Ensure memory accesses above are completed before DMA is started
	 *
	 * Replacing __DSB() with __DMB() is also OK in this case.
	 *
	 * Refer to "multi-master systems" section with DMA in:
	 * https://static.docs.arm.com/dai0321/a/DAI0321A_programming_guide_memory_barriers_for_m_profile.pdf
	 */
	//__DSB();
}

static bool crypto_submodule_wait(volatile uint16_t *submodule_done, int32_t timeout_us)
{	uint32_t currTick;
	if(timeout_us < 0) 
	{	timeout_us = 0x7FFFFFFF;
	}
	
	currTick = GetTimeTicks();
	while(! *submodule_done)
	{	if((GetTimeTicks() - currTick) > (timeout_us / 1000))
		{	break;
		}
	}

	/* Ensure while loop above and subsequent code are not reordered */
	//__DSB();

	if((*submodule_done & CRYPTO_DONE_OK)) 
	{	/* Done with OK */
		return true;
	} 
	else if ((*submodule_done & CRYPTO_DONE_ERR)) 
	{	/* Done with error */
		return false;
	}
	return false;
}

/* Crypto interrupt handler */
void CRPT_IRQHandler()
{	uint32_t intsts;

	if((intsts = PRNG_GET_INT_FLAG(CRPT)) != 0) 
	{	/* Done with OK */
		if(intsts & CRPT_INTSTS_PRNGIF_Msk) 
		{	/* Done with OK */
			crypto_prng_done |= NU_CRYPTO_DONE_OK;
		} 
		else if(intsts & CRPT_INTSTS_PRNGEIF_Msk) 
		{	/* Done with error */
			crypto_prng_done |= NU_CRYPTO_DONE_ERR;
		}
		/* Clear interrupt flag */
		PRNG_CLR_INT_FLAG(CRPT);
	}  
	else if((intsts = AES_GET_INT_FLAG(CRPT)) != 0) 
	{	/* Done with OK */
		if(intsts & CRPT_INTSTS_AESIF_Msk) 
		{	/* Done with OK */
			crypto_aes_done |= NU_CRYPTO_DONE_OK;
		} 
		else if(intsts & CRPT_INTSTS_AESEIF_Msk) 
		{	/* Done with error */
			crypto_aes_done |= NU_CRYPTO_DONE_ERR;
		}
		/* Clear interrupt flag */
		AES_CLR_INT_FLAG(CRPT);
	} 
#ifdef MBEDTLS_DES_ALT
	else if ((intsts = TDES_GET_INT_FLAG(CRPT)) != 0) 
	{	/* Done with OK */
		crypto_des_done |= CRYPTO_DONE_OK;
		/* Clear interrupt flag */
		TDES_CLR_INT_FLAG(CRPT);
	} 
#endif
	else if((intsts = ECC_GET_INT_FLAG(CRPT)) != 0) 
	{	/* Check interrupt flags */
		if(intsts & CRPT_INTSTS_ECCIF_Msk) 
		{	/* Done with OK */
			crypto_ecc_done |= CRYPTO_DONE_OK;
		} 
		else if(intsts & CRPT_INTSTS_ECCEIF_Msk) 
		{	/* Done with error */
			crypto_ecc_done |= CRYPTO_DONE_ERR;
		}
		ECC_DriverISR(CRPT);
		/* Clear interrupt flag */
		//ECC_CLR_INT_FLAG(CRPT);
	}
#ifdef MBEDTLS_RSA_ALT
	else if((intsts == RSA_GET_INT_FLAG(CRPT)) != 0)
	{	if(intsts & CRPT_INTSTS_RSAIF_Msk)
		{	crypto_rsa_done |= CRYPTO_DONE_OK;
		}
		else if(intsts & CRPT_INTSTS_RSAEIF_Msk)
		{	crypto_rsa_done |= CRYPTO_DONE_ERR;
		}
		RSA_CLR_INT_FLAG(CRPT);
	}
#endif
}

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{	size_t idx;
	unsigned int keySeed = 0;
	int remainSize;
	
#if !defined TEST_SOFTWARE_CRYPT
	crypto_init();
	RNG_Open();
	
	remainSize = len;
	for(idx=0;idx<len;idx++)
	{	RNG_Random(&keySeed, 1);
		output[idx] = keySeed;
	}
	
	*olen = len;
	crypto_uninit();
#else
	crypto_init();
	RNG_Open();
	for(idx=0;idx<len;idx++)
	{	output[idx] = idx;
	}
	*olen = len;
	crypto_uninit();
#endif
	return 0;
}


int genEcdhPrivateKeyInKs(void)
{	uint32_t au32ECC_N[18] = {0};
	int32_t i;
	int32_t err;
	int32_t privateKeyIdx;

	SYS_UnlockReg();
	CLK_EnableModuleClock(CRPT_MODULE);
	CLK_EnableModuleClock(KS_MODULE);
	
	KS_Open();

	NVIC_EnableIRQ(CRPT_IRQn);
	
	ECC_ENABLE_INT(CRPT);
	SHA_ENABLE_INT(CRPT);

	for(i = 0; i < 18; i++)
		au32ECC_N[i] = 0;
	au32ECC_N[7] = 0xFFFFFFFFul;
	au32ECC_N[6] = 0x00000001ul;
	au32ECC_N[5] = 0x00000000ul;
	au32ECC_N[4] = 0x00000000ul;
	au32ECC_N[3] = 0x00000000ul;
	au32ECC_N[2] = 0xFFFFFFFFul;
	au32ECC_N[1] = 0xFFFFFFFFul;
	au32ECC_N[0] = 0xFFFFFFFFul;
	
	err = RNG_ECDH_Init(PRNG_KEY_SIZE_256, au32ECC_N);
	if(err)
	{	dbgMsg("%s", "PRNG ECDH Inital failed\r\n");
		return -1;
	}
	privateKeyIdx = RNG_ECDH(PRNG_KEY_SIZE_256);
	if(privateKeyIdx < 0)
	{	dbgMsg("%s", "Fail to write private key to KS SRAM");
		return -1;
	}
	
	NVIC_DisableIRQ(CRPT_IRQn);
	ECC_DISABLE_INT(CRPT);
	SHA_DISABLE_INT(CRPT);
	//SYS_UnlockReg();    // Unlock protected register
	CLK_DisableModuleClock(CRPT_MODULE);
	CLK_DisableModuleClock(KS_MODULE);
	
	return privateKeyIdx;
}

#ifdef MBEDTLS_ECDH_GEN_PUBLIC_ALT
//#include "m2354_mbedtls_config.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/asn1write.h"

#define B2C(c)    (((uint8_t)(c)<10)?((uint8_t)(c)+'0'):((uint8_t)(c)-10+'a'))

unsigned char fixPrivateKey[] = 
{	0x65, 0x37, 0x39, 0x61, 0x37, 0x66, 0x32, 0x63, 0x36, 0x64, 0x63, 0x36, 0x64, 0x32, 0x37, 0x61,
	0x38, 0x62, 0x34, 0x39, 0x32, 0x61, 0x61, 0x65, 0x35, 0x38, 0x39, 0x31, 0x31, 0x62, 0x37, 0x33,
	0x63, 0x66, 0x66, 0x62, 0x63, 0x30, 0x63, 0x63, 0x36, 0x31, 0x32, 0x30, 0x65, 0x30, 0x64, 0x39,
	0x61, 0x66, 0x31, 0x61, 0x33, 0x36, 0x64, 0x37, 0x61, 0x32, 0x63, 0x39, 0x30, 0x39, 0x30, 0x39
};

void GenPrivateKeyFix(char *d, uint32_t eccCurveType, uint32_t u32NBits)
{	memcpy(d, fixPrivateKey, 0x40);
}

void GenPrivateKey(char *d, uint32_t eccCurveType, uint32_t u32NBits)
{	uint8_t *u8r;
	int32_t i, j;
	uint32_t u32Idx;
	uint32_t *au32r;	//[(ECC_KEY_SIZE + 31) / 32];
	
	au32r = (uint32_t *)SENS_MEM_ZALLOC((ROUNDUP(u32NBits, 32)/32) * sizeof(uint32_t));
	u8r = (uint8_t *)&au32r[0];
	do
	{	/* Generate random number for private key */
		RNG_Random(au32r, (u32NBits + 31) / 32);

		for(u32Idx = 0, j = 0; u32Idx < (u32NBits + 7) / 8; u32Idx++)
		{	d[j++] = B2C(u8r[u32Idx] & 0xf);
			d[j++] = B2C(u8r[u32Idx] >> 4);
		}
		d[(u32NBits + 3) / 4] = 0;

		/* Check if the private key valid */
		if(ECC_IsPrivateKeyValid(CRPT, eccCurveType, d))
		{	break;
		}
		else
		{	/* Decrease 1 bit and try again */
			d[(u32NBits + 2) / 4] = 0;
			if(ECC_IsPrivateKeyValid(CRPT, eccCurveType, d))
			{	if(((u32NBits & 0x3) != 0) && (((u32NBits - 1) & 0x3) == 0))
				{	/* Need padding 1 nibble back */
					j = (u32NBits + 2) / 4;
					for(i = j; i > 0; i--)
						d[i] = d[i - 1];
					d[i + 1] = 0;
					d[0] = '0';
				}
				break;
			}
			else
			{	/* Invalid key */
				dbgMsg("%s", "Current private key is not valid. Need a new one.\r\n");
			}
		}
	}while(1);
	SENS_MEM_FREE(au32r);
}

int mbedtls_ecdh_gen_public(mbedtls_ecp_group *grp, mbedtls_mpi *d, mbedtls_ecp_point *Q, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng) 
{	int returnStatus = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
	uint8_t *publicKey=NULL;
	uint32_t eccCurveType;
	uint32_t keySize;
	char *publicKeyQx = NULL;
	char *publicKeyQy = NULL;
	uint8_t *publicKeyQxHex = NULL; 
	uint8_t *publicKeyQyHex = NULL;
	//char asciiToHex[3];
	uint32_t publicKeyQxLen, publicKeyQyLen;
	uint32_t idx;
	size_t plen;
	uint32_t startTick, endTick;
	//char *end;
	
	
#if 0
	displayBufInHex((uint8_t *)grp, sizeof(mbedtls_ecp_group), __func__, __LINE__);
	
	dbgMsg("mbedtls_ecp_group P, len:%d\r\n", grp->P.n);
	if(grp->P.n)
		displayBufInHex((uint8_t *)grp->P.p, grp->P.n*4, __func__, __LINE__);
	dbgMsg("mbedtls_ecp_group A, len:%d\r\n", grp->A.n);
	if(grp->A.n)
		displayBufInHex((uint8_t *)grp->A.p, grp->A.n*4, __func__, __LINE__);
	dbgMsg("mbedtls_ecp_group B, len:%d\r\n", grp->B.n);
	if(grp->B.n)
		displayBufInHex((uint8_t *)grp->B.p, grp->B.n*4, __func__, __LINE__);
	dbgMsg("mbedtls_ecp_group N, len:%d\r\n", grp->N.n);
	if(grp->N.n)
		displayBufInHex((uint8_t *)grp->N.p, grp->N.n*4, __func__, __LINE__);
	dbgMsg("mbedtls_ecp_group G->X, len:%d\r\n", grp->G.X.n);
	if(grp->G.X.n)
		displayBufInHex((uint8_t *)grp->G.X.p, grp->G.X.n*4, __func__, __LINE__);
	dbgMsg("mbedtls_ecp_group G->Y, len:%d\r\n", grp->G.Y.n);
	if(grp->G.Y.n)
		displayBufInHex((uint8_t *)grp->G.Y.p, grp->G.Y.n*4, __func__, __LINE__);
	dbgMsg("mbedtls_ecp_group G->Z, len:%d\r\n", grp->G.Z.n);
	if(grp->G.Z.n)
		displayBufInHex((uint8_t *)grp->G.Z.p, grp->G.Z.n*4, __func__, __LINE__);
	
	if(grp->T)
	{	dbgMsg("mbedtls_ecp_group T->X, len:%d\r\n", grp->T->X.n);
		if(grp->T->X.n)
			displayBufInHex((uint8_t *)grp->T->X.p, grp->T->X.n*4, __func__, __LINE__);
		dbgMsg("mbedtls_ecp_group T->Y, len:%d\r\n", grp->T->Y.n);
		if(grp->T->Y.n)
			displayBufInHex((uint8_t *)grp->T->Y.p, grp->T->Y.n*4, __func__, __LINE__);
		dbgMsg("mbedtls_ecp_group T->Z, len:%d\r\n", grp->T->Z.n);
		if(grp->T->Z.n)
			displayBufInHex((uint8_t *)grp->T->Z.p, grp->T->Z.n*4, __func__, __LINE__);
	}
	
	dbgMsg("mbedtls_mpi s:%d\r\n", d->s);
	displayBufInHex((uint8_t *)d->p, d->n, __func__, __LINE__);
	plen = mbedtls_mpi_size( &grp->P );
	dbgMsg("plen: %d, mbedtls_ecp_get_type( grp ):%d\r\n", plen, mbedtls_ecp_get_type( grp ));
#endif
	
	crypto_init();
	ECC_ENABLE_INT(CRPT);
	RNG_Open();
	
	switch(grp->id)
	{	case MBEDTLS_ECP_DP_SECP256R1:	eccCurveType = CURVE_P_256;	keySize = 256;	break;
		case MBEDTLS_ECP_DP_SECP192R1:	
		case MBEDTLS_ECP_DP_SECP224R1:	
    case MBEDTLS_ECP_DP_SECP384R1:	
		case MBEDTLS_ECP_DP_SECP521R1:	
    case MBEDTLS_ECP_DP_BP256R1:		
    case MBEDTLS_ECP_DP_BP384R1:		
    case MBEDTLS_ECP_DP_BP512R1:		
    case MBEDTLS_ECP_DP_SECP224K1:	
    case MBEDTLS_ECP_DP_SECP256K1:	
    case MBEDTLS_ECP_DP_CURVE448:		
		case MBEDTLS_ECP_DP_NONE:
		case MBEDTLS_ECP_DP_CURVE25519:
		case MBEDTLS_ECP_DP_SECP192K1:
		defalut:
				returnStatus = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
				goto cleanup;
	}
	
	if(privateKeyD == NULL)
		privateKeyD = SENS_MEM_ZALLOC(168);
	publicKeyQx = SENS_MEM_ZALLOC(168);
	publicKeyQy = SENS_MEM_ZALLOC(168);
	
	startTick = GetTimeTicks();
	if(sensSys->keysInf.ecdhPrivateKeyIdx < 0)
	{	if(!privateKeyCreated)
		{	//GenPrivateKey(privateKeyD, eccCurveType, keySize);
			GenPrivateKeyFix(privateKeyD, eccCurveType, keySize);
			privateKeyCreated = 1;
		}
	}
	endTick = GetTimeTicks();
	dbgMsg("gen private key, start:%d, end:%d, private key data:\r\n", startTick, endTick);
	displayBufInHex((uint8_t *)privateKeyD, 168, __func__, __LINE__);
	
	startTick = GetTimeTicks();
	
	if(sensSys->keysInf.ecdhPrivateKeyIdx < 0)
	{	if(ECC_GeneratePublicKey(CRPT, eccCurveType, privateKeyD, publicKeyQx, publicKeyQy) < 0)
		{	returnStatus = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
			goto cleanup;
		}
	}
	else
	{	SYS_UnlockReg();
		CLK_EnableModuleClock(KS_MODULE);
		
		if(ECC_GeneratePublicKey_KS(CRPT, eccCurveType, KS_SRAM, sensSys->keysInf.ecdhPrivateKeyIdx, publicKeyQx, publicKeyQy, 0) < 0)
		{	returnStatus = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
			goto cleanup;
		}
	}
	endTick = GetTimeTicks();
	dbgMsg("gen public key, start:%d, end:%d\r\n", startTick, endTick);
	dbgMsg("%s", "\r\npublic key x data:\r\n");
	displayBufInHex((uint8_t *)publicKeyQx, strlen(publicKeyQx), __func__, __LINE__);
	dbgMsg("%s", "\r\npublic key y data:\r\n");
	displayBufInHex((uint8_t *)publicKeyQy, strlen(publicKeyQy), __func__, __LINE__);
	
	publicKeyQxLen = strlen(publicKeyQx) / 2;
	publicKeyQyLen = strlen(publicKeyQy) / 2;
	
	publicKeyQxHex = SENS_MEM_ZALLOC(publicKeyQxLen);
	publicKeyQyHex = SENS_MEM_ZALLOC(publicKeyQyLen);
	publicKey = SENS_MEM_ZALLOC(publicKeyQxLen + publicKeyQyLen + 1);
	
	publicKey[0] = 0x04;
	CRPT_Hex2Reg(publicKeyQx, (uint32_t *)publicKeyQxHex);
	CRPT_Hex2Reg(publicKeyQy, (uint32_t *)publicKeyQyHex);

	dbgMsg("%s", "\r\npublic key x Hex data:\r\n");
	displayBufInHex((uint8_t *)publicKeyQxHex, publicKeyQxLen, __func__, __LINE__);
	dbgMsg("%s", "\r\npublic key y Hex data:\r\n");
	displayBufInHex((uint8_t *)publicKeyQyHex, publicKeyQyLen, __func__, __LINE__);
	/*for(idx=0;idx<publicKeyQxLen;idx++)
	{	publicKey[1+idx] = publicKeyQxHex[publicKeyQxLen-idx-1];
	}
	for(idx=0;idx<publicKeyQyLen;idx++)
	{	publicKey[1+idx+publicKeyQxLen] = publicKeyQyHex[publicKeyQyLen-idx-1];
	}*/
	memcpy(&publicKey[1], publicKeyQxHex, publicKeyQxLen);
	memcpy(&publicKey[1+publicKeyQxLen], publicKeyQyHex, publicKeyQyLen);
	
	SENS_MEM_FREE(publicKeyQxHex);
	SENS_MEM_FREE(publicKeyQyHex);
	/*asciiToHex[2] = 0;
	
	for(idx=0;idx<publicKeyQxLen;idx++)
	{	asciiToHex[0] = publicKeyQx[idx*2];
		asciiToHex[1] = publicKeyQx[idx*2+1];
		publicKey[1+idx] = (uint8_t)strtol(asciiToHex, &end, 16);
	}
	
	for(idx=0;idx<publicKeyQyLen;idx++)
	{	asciiToHex[0] = publicKeyQy[idx*2];
		asciiToHex[1] = publicKeyQy[idx*2+1];
		publicKey[1+idx+publicKeyQxLen] = (uint8_t)strtol(asciiToHex, &end, 16);
	}*/
	dbgMsg("%s", "\r\npublic key data:\r\n");
	displayBufInHex((uint8_t *)publicKey, publicKeyQxLen + publicKeyQyLen + 1, __func__, __LINE__);
	if(mbedtls_ecp_point_read_binary(grp, Q, (uint8_t *)publicKey, (publicKeyQxLen + publicKeyQyLen + 1)) != 0)
	{	returnStatus = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
		goto cleanup;
	}
	dbgMsg("%s", "\r\nQ->x data:\r\n");
	displayBufInHex((uint8_t *)Q->X.p, Q->X.n * 4, __func__, __LINE__);
	dbgMsg("%s", "\r\nQ->y data:\r\n");
	displayBufInHex((uint8_t *)Q->Y.p, Q->Y.n * 4, __func__, __LINE__);
	
	returnStatus = 0;
	//returnStatus = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
cleanup:
	if(publicKey)
		SENS_MEM_FREE(publicKey);
	//if(privateKeyD)
	//	SENS_MEM_FREE(privateKeyD);
	if(publicKeyQx)
		SENS_MEM_FREE(publicKeyQx);
	if(publicKeyQy)
		SENS_MEM_FREE(publicKeyQy);
	ECC_DISABLE_INT(CRPT);
	SYS_UnlockReg();    // Unlock protected register
	CLK_DisableModuleClock(KS_MODULE);
	crypto_uninit();
	return returnStatus;
}
#endif

#ifdef MBEDTLS_ECDH_COMPUTE_SHARED_ALT
int mbedtls_ecdh_compute_shared(mbedtls_ecp_group *grp, mbedtls_mpi *z, const mbedtls_ecp_point *Q, const mbedtls_mpi *d, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{	int returnStatus = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
	PUBLIC_KEY_FROM_HOST pk;
	size_t pkSize;
	uint8_t	*pkOut=NULL;
	uint32_t eccCurveType;
	uint32_t keySize;
	char *shareKey=NULL;
	char *publicKeyQx = NULL;
	char *publicKeyQy = NULL;
	uint8_t *publicKeyQxHex = NULL;
	uint8_t *publicKeyQyHex = NULL;
	uint8_t *shareKeyHex = NULL;
	uint32_t idx;
	uint32_t pLen;
	char *bufPtr;
	int32_t i32ShareKeyIdx;
	
	crypto_init();
	ECC_ENABLE_INT(CRPT);
	RNG_Open();
	
	switch(grp->id)
	{	case MBEDTLS_ECP_DP_SECP256R1:	eccCurveType = CURVE_P_256;	keySize = 256;	break;
		case MBEDTLS_ECP_DP_SECP192R1:	
		case MBEDTLS_ECP_DP_SECP224R1:	
    case MBEDTLS_ECP_DP_SECP384R1:	
		case MBEDTLS_ECP_DP_SECP521R1:	
    case MBEDTLS_ECP_DP_BP256R1:		
    case MBEDTLS_ECP_DP_BP384R1:		
    case MBEDTLS_ECP_DP_BP512R1:		
    case MBEDTLS_ECP_DP_SECP224K1:	
    case MBEDTLS_ECP_DP_SECP256K1:	
    case MBEDTLS_ECP_DP_CURVE448:		
		case MBEDTLS_ECP_DP_NONE:
		case MBEDTLS_ECP_DP_CURVE25519:
		case MBEDTLS_ECP_DP_SECP192K1:
		defalut:
				returnStatus = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
				goto cleanup;
	}
	
	pk.keyType = eccCurveType;
	
	pkOut = SENS_MEM_ZALLOC(100);
	shareKey = SENS_MEM_ZALLOC(168);
	
	mbedtls_ecp_point_write_binary(grp, Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &pkSize, pkOut, 100);
	
	dbgMsg("%s", "Q->X:\r\n");
	displayBufInHex((uint8_t *)Q->X.p, Q->X.n*4, __func__, __LINE__);
	dbgMsg("%s", "Q->Y:\r\n");
	displayBufInHex((uint8_t *)Q->Y.p, Q->Y.n*4, __func__, __LINE__);
	
	dbgMsg("%s", "pkout Data:\r\n");
	displayBufInHex((uint8_t *)pkOut, pkSize, __func__, __LINE__);
	
	pLen = (pkSize-1)/2;
	publicKeyQxHex = SENS_MEM_ZALLOC(pLen);
	publicKeyQyHex = SENS_MEM_ZALLOC(pLen);
	
	memcpy(publicKeyQxHex, &pkOut[1], pLen);
	memcpy(publicKeyQyHex, &pkOut[1+pLen], pLen);
	
	dbgMsg("%s", "publicKeyQxHex Data:\r\n");
	displayBufInHex((uint8_t *)publicKeyQxHex, pLen, __func__, __LINE__);
	dbgMsg("%s", "publicKeyQyHex Data:\r\n");
	displayBufInHex((uint8_t *)publicKeyQyHex, pLen, __func__, __LINE__);
	
	publicKeyQx = SENS_MEM_ZALLOC(pLen*2+1);
	publicKeyQy = SENS_MEM_ZALLOC(pLen*2+1);
	
	CRPT_Reg2Hex(pLen*2, (uint32_t *)publicKeyQxHex, publicKeyQx);
	CRPT_Reg2Hex(pLen*2, (uint32_t *)publicKeyQyHex, publicKeyQy);
	
	dbgMsg("%s", "publicKeyQx Data:\r\n");
	displayBufInHex((uint8_t *)publicKeyQx, strlen(publicKeyQx), __func__, __LINE__);
	dbgMsg("%s", "publicKeyQy Data:\r\n");
	displayBufInHex((uint8_t *)publicKeyQy, strlen(publicKeyQy), __func__, __LINE__);
	
	//pk.publicKey = pkOut;
	//pk.length = pkSize;
	if(sensSys->keysInf.ecdhPrivateKeyIdx < 0)
	{	ECC_GenerateSecretZ(CRPT, eccCurveType, privateKeyD, publicKeyQx, publicKeyQy, shareKey);
	}
	else
	{	SYS_UnlockReg();
		CLK_EnableModuleClock(KS_MODULE);
		i32ShareKeyIdx = ECC_GenerateSecretZ_KS(CRPT, eccCurveType, KS_SRAM, sensSys->keysInf.ecdhPrivateKeyIdx, publicKeyQx, publicKeyQy);
		
		KS_Read(KS_SRAM, i32ShareKeyIdx, (uint32_t *)shareKey, pLen * 2);
	}
	dbgMsg("%s", "shareKey Data:\r\n");
	displayBufInHex((uint8_t *)shareKey, strlen(shareKey), __func__, __LINE__);
	
	shareKeyHex = SENS_MEM_ZALLOC(strlen(shareKey));
	CRPT_Hex2Reg(shareKey, (uint32_t *)shareKeyHex);
	dbgMsg("%s", "shareKeyHex Data:\r\n");
	displayBufInHex((uint8_t *)shareKeyHex, strlen(shareKey), __func__, __LINE__);
	
	mbedtls_mpi_read_binary( z, shareKeyHex, mbedtls_mpi_size( &grp->P ) );
	
	dbgMsg("%s", "Z Data:\r\n");
	displayBufInHex((uint8_t *)z->p, z->n * 4, __func__, __LINE__);
	
	returnStatus = 0;
	
cleanup:
	if(pkOut)
		SENS_MEM_FREE(pkOut);
	if(shareKey)
		SENS_MEM_FREE(shareKey);
	if(shareKeyHex)
		SENS_MEM_FREE(shareKeyHex);
	if(publicKeyQxHex)
		SENS_MEM_FREE(publicKeyQxHex);
	if(publicKeyQx)
		SENS_MEM_FREE(publicKeyQx);
	if(publicKeyQyHex)
		SENS_MEM_FREE(publicKeyQyHex);
	if(publicKeyQy)
		SENS_MEM_FREE(publicKeyQy);
	ECC_DISABLE_INT(CRPT);
	SYS_UnlockReg();    // Unlock protected register
	CLK_DisableModuleClock(KS_MODULE);
	crypto_uninit();
	return returnStatus;
}
#endif
