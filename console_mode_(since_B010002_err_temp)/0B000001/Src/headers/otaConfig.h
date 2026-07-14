#ifndef __OTA_CONFIG_H__
#define __OTA_CONFIG_H__

#include "sensminiCfg.h"

#define OTA_FILE_NAME		"ota.bin"
#define OTA_TEMP_FILE_NAME	"otafwTemp.bin"
#define OTA_CHKSUM_NAME		"crc.bin"

#define ETH_OTA_LOAD_SIZE_PER_API				4096
#define MOBILE_OTA_LOAD_SIZE_PER_API			512
#define MOBILE_OTA_LOAD_SIZE_PER_API_LOW_RSSI	512
#define MOBILE_OTA_LOAD_SIZE_PER_API_HIGH_RSSI	2048

#define FS_LOAD_MAX_SIZE	                    4096
#define FS_LOAD_MAX_SIZE_TELIT                  1024
#define MAX_LOAD_SEGMENT_SIZE_TELIT				(4096 * 10)
#define MAX_LOAD_SEGMENT_SIZE_STR_LENGTH		5

#define FOTA_HTTP_ERR_OK						0
#define FOTA_HTTP_ERR_CONNECT					-1
#define FOTA_HTTP_ERR_XMIT						-2
#define FOTA_HTTP_ERR_RSP_TIMEOUT				-3


#define FW_LOAD_SUCCESS_ADDR					0x2000	                //sector 2
#define FW_EXPIRE_ADDR							0x4000
#define RESOURCE_BASE_ADDRESS					0x140000
#define ANASYSTEM_BOOT_INDICATE_STR				"anasystem"
#define ANA_SIG									"\x55\xAA"
#define RESOURCE_VERSION_INDEX					0x01
#define RESOURCE_MAX_SIZE						(3145728)               //3MB
#define RESOURCE_TYPE_FIRMWARE					0x21
#define SUBTYPE_FIRMWARE_CHECKSUM				0x01
#define SUBTYPE_FIRMWARE						0x02
#define BACKUP_FW_MAX_SIZE						(512*1024)
#define ENTRY_MAX_SIZE			            	(4096)
#define BACKUP_CHKSUM_MAX_SIZE		      		(4096)
#define RESOURCE_ENTRY_SIZE		          		(4096)
#define BPB_MAX_SIZE			              	(4096)

typedef struct _FIRMWARE_INFO	// should 2 info , 1 is bin file, 1 is checksum file
{
	//uint8_t type;				                                        // 0x11
	//uint8_t subtype;			                                        // 0x01: checksum file, 0x02: bin file
	//uint16_t isValid;
	//char name[16];
	//uint32_t startAddr;
	//uint32_t romStartAddr;
	uint32_t currWriteOffset;
	uint32_t length;
}FIRMWARE_INFO, firmware_info_t;

typedef struct anasystem_filesystem
{	char tag[9];	                                                                // anasystem
	uint8_t reserved[6];
	uint8_t structVerIndex;
	uint8_t signature[2];	                                                        // 0x55, 0xAA
	uint16_t resourceCount;	                                                // maybe http file
	uint32_t resoruceInfoOffset;	                                                // 4096	//4K
	uint32_t firmwareInfoOffset;	                                                // 8192	//4K + 4K, for sector erase
	uint32_t firmwareBakInfoOffset;	                                        // 8192	//4K + 4K, for sector erase
}ANA_FILESYSTEM, anasystem_filesystem_t;


#if SUPPORT_FOTA_MD5
typedef struct file_md5
{	uint16_t sig;
	uint16_t ver;
	uint32_t cpuModel;
	uint8_t  md5[16];
}FILE_MD5, file_md5_t;
#endif

#endif