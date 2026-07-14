#ifndef __AD7124_ADC_H__
#define __AD7124_ADC_H__

#include "sensminiCfg.h"
#include "sensCommon.h"
#include "driver/SENS_SPI.h"

#define ADC_SPI_CHANNEL	1

#define SINGLE_CONVERSION_SEVERAL_CHANNELS_EN 0
#define SINGLE_CONVERSION_SINGLE_CHANNEL_EN 1

//#define SENS_SPI_T				SPI_T
//#define SENS_SPI_IS_BUSY		SPI_IS_BUSY
//#define SENS_SPI_CLOSE			SPI_Close
#define SENS_ADC_SPI_MODE		SPI_MODE_SPI	
#if ADC_SPI_CHANNEL == 0
#define SENS_ADC_SPI_MODULE	SPI0_MODULE
#define SENS_ADC_SPI_CLK_SEL	CLK_CLKSEL2_SPI0SEL_PLL_DIV2
#define SENS_ADC_SPI_CTX		SPI0
#elif ADC_SPI_CHANNEL == 1
#define SENS_ADC_SPI_MODULE		SPI1_MODULE
#define SENS_ADC_SPI_CLK_SEL	CLK_CLKSEL2_SPI1SEL_PLL_DIV2	//CLK_CLKSEL2_SPI1SEL_PLL	
#define SENS_ADC_SPI_CTX		SPI1
#else
#error "SPI Module must set to 0 or 1"
#endif


#define REG_COMMS		0x00
#define REG_STATUS		0x00
#define REG_ADC_CTRL	0x01
#define REG_ADC_DATA	0x02	//define in ADC.h
#define REG_IO_CTRL_1	0x03
#define REG_IO_CTRL_2	0x04
#define REG_ID			0x05
#define REG_ERR			0x06
#define REG_ERR_EN		0x07
#define REG_MCLK_CNT	0x08
#define REG_CHANNEL_0	0x09
#define REG_CHANNEL_1	0x0A
#define REG_CHANNEL_2	0x0B
#define REG_CHANNEL_3	0x0C
#define REG_CHANNEL_4	0x0D
#define REG_CHANNEL_5	0x0E
#define REG_CHANNEL_6	0x0F
#define REG_CHANNEL_7	0x10
#define REG_CHANNEL_8	0x11
#define REG_CHANNEL_9	0x12
#define REG_CHANNEL_10	0x13
#define REG_CHANNEL_11	0x14
#define REG_CHANNEL_12	0x15
#define REG_CHANNEL_13	0x16
#define REG_CHANNEL_14	0x17
#define REG_CHANNEL_15	0x18
#define REG_CONFIG_0	0x19
#define REG_CONFIG_1	0x1A
#define REG_CONFIG_2	0x1B
#define REG_CONFIG_3	0x1C
#define REG_CONFIG_4	0x1D
#define REG_CONFIG_5	0x1E
#define REG_CONFIG_6	0x1F
#define REG_CONFIG_7	0x20
#define REG_FILTER_0	0x21
#define REG_FILTER_1	0x22
#define REG_FILTER_2	0x23
#define REG_FILTER_3	0x24
#define REG_FILTER_4	0x25
#define REG_FILTER_5	0x26
#define REG_FILTER_6	0x27
#define REG_FILTER_7	0x28
#define REG_OFFSET_0	0x29
#define REG_OFFSET_1	0x2A
#define REG_OFFSET_2	0x2B
#define REG_OFFSET_3	0x2C
#define REG_OFFSET_4	0x2D
#define REG_OFFSET_5	0x2E
#define REG_OFFSET_6	0x2F
#define REG_OFFSET_7	0x30
#define REG_GAIN_0		0x31
#define REG_GAIN_1		0x32
#define REG_GAIN_2		0x33
#define REG_GAIN_3		0x34
#define REG_GAIN_4		0x35
#define REG_GAIN_5		0x36
#define REG_GAIN_6		0x37
#define REG_GAIN_7		0x38
//#define REG_RESET		0xFF
#define REG_RESET		0x39

enum ADC_REG_RW_FLAG
{	ADC_REG_READ_ONLY,
	ADC_REG_WRITE_ONLY,
	ADC_REG_READ_WRITE
};

enum
{	DIFFERENTIAL_MODE = 0,
	SINGLE_END_MODE
};

typedef struct _ADC_REG_INFO
{	uint8_t regAddr;
	uint8_t size;
	uint8_t waitFlag;
	enum ADC_REG_RW_FLAG RW;
	uint32_t value;
	
}ADC_REG_INFO;

typedef union _ADC_STATUS_FLAG
{	uint8_t status;
	struct
	{	uint8_t activeCh:4;
		uint8_t porFlag:1;
		uint8_t	reserved:1;
		uint8_t error:1;
		uint8_t nRdy:1;
	};
}ADC_STATUS_FLAG;

//COMMS
#define CR_READ		(1 << 6)
#define CR_RS(r)	(r & 0x3F)

#define WEN_OFFSET		7
#define RW_OFFSET		6
#define RS_OFFSET		0
#define RS_MASK	0x3F
#define SET_WEN		(1 << WEN_OFFSET)
#define CLR_WEN		~SET_WEN
#define GET_WEN(x)
#define SET_RW		(1 << RW_OFFSET)

//STATUS
#define RSY			0x80
#define ERROR_FLAG	0x40
#define POR_FLAG	0x10
#define CH_ACTIVE	0x00
#define CH_ACTIVE_MASK	0x0F

//ADC_CTRL
#define DOUT_RDY_DEL	0x1000
#define CONT_READ		0x0800
#define DATA_STATUS		0x0400
#define CS_EN			0x0200
#define REF_EN			0x0100
//#define POWER_MODE		0x00
#define DOUT_RDY_DEL_OFFSET	12
#define DOUT_RDY_DEL_MASK	0xEFFF
#define CLR_DOUT_RDY_DEL(x)	x &= DOUT_RDY_DEL_MASK;
#define SET_DOUT_RDY_DEL(x)	CLR_DOUT_RDY_DEL(x);	\
							x |= (1 << DOUT_RDY_DEL_OFFSET)

#define CONT_READ_OFFSET	11
#define CONT_READ_MASK		0xF7FF
#define CLR_CONT_READ(x)	x &= CONT_READ_MASK
#define SET_CONT_READ(x)	CLR_CONT_READ(x);	\
							x |= (1 << CONT_READ_OFFSET)

#define DATA_STATUS_OFFSET	10
#define DATA_STATUS_MASK	0xFBFF
#define CLR_DATA_STATUS(x)	x &= DATA_STATUS_MASK
#define SET_DATA_STATUS(x)	CLR_DATA_STATUS(x);	\
							x |= (1 << DATA_STATUS_OFFSET)

#define CS_EN_OFFSET	9
#define CS_EN_MASK		0xFDFF
#define CLR_CS_EN(x)	x &= CS_EN_MASK
#define SET_CS_EN(x)	CLR_CS_EN(x);	\
						x |= (1 << CS_EN_OFFSET)

#define REF_EN_OFFSET	8
#define REF_EN_MASK		0xFEFF
#define CLR_REF_EN(x)	x &= REF_EN_MASK
#define SET_REF_EN(x)	CLR_REF_EN(x);\
						x |= (1 << REF_EN_OFFSET)

#define POWER_MODE_OFFSET	6
#define POWER_MODE_LOW		0
#define POWER_MODE_MID		1
#define POWER_MODE_FULL		2
#define POWER_MODE_FULL_1	3
#define POWER_MASK		0xFF3F
#define CLR_POWER_MODE(x)		x &= POWER_MASK
#define SET_POWER_MODE(x, y)	CLR_POWER_MODE(x);	\
								x |= (y << POWER_MODE_OFFSET)

#define MODE_OFFSET	2
#define MODE_CONT_CONVERSION		0
#define MODE_SINGLE_CONVERSION		1
#define MODE_STANDBY				2
#define MODE_POWER_DOWN				3
#define MODE_IDLE					4
#define MODE_INTERNAL_ZERO_SCALE	5
#define MODE_INTERNAL_FULL_SCALE	6
#define MODE_SYSTEM_ZERO_SCALE		7
#define MODE_SYSTEM_FULL_SCALE		8
#define MODE_MASK	0xFFC3
#define CLR_MODE(x)			x &= MODE_MASK
#define SET_MODE(x, y)		CLR_MODE(x);	\
							x |= (y << MODE_OFFSET)
#define GET_MODE(x)			x &= ~MODE_MASK;	\
							x = x >> MODE_OFFSET

#define CHANNEL_ENABLE	0x8000
#define CHANNEL_EN_OFFSET	15
#define CHANNEL_EN_MASK		0x7FFF
#define CLR_CHANNEL_EN(x)	x &= CHANNEL_EN_MASK
#define SET_CHANNEL_EN(x)	CLR_CHANNEL_EN(x);	\
							x |= (1 << CHANNEL_EN_OFFSET)

#define SETUP_OFFSET	12
#define SETUP_MASK	0x8FFF
#define GET_SETUP(x)	(x & (~SETUP_MASK)) >> 12
#define CLR_SETUP(x)	x &= SETUP_MASK
#define SET_SETUP(x, y)	CLR_SETUP(x);	\
						x |= (y <<SETUP_OFFSET)
#define AINP_OFFSET	5
#define AINM_OFFSET	0
#define AINP_MASK	0xFC1F
#define AINM_MASK	0xFFE0
#define AIN0			0
#define AIN1			1
#define AIN2			2
#define AIN3			3
#define AIN4			4
#define AIN5			5
#define AIN6			6
#define AIN7			7
#define AIN8			8
#define AIN9			9
#define AIN10			10
#define AIN11			11
#define AIN12			12
#define AIN13			13
#define AIN14			14
#define AIN15			15
#define TEMPERATURE		16
#define AVSS			17
#define INTERNAL_IF		18
#define DGND			19
#define AVDD_AVSS_P		20
#define AVDD_AVSS_M		21
#define IOVDD_DGND_P	22
#define IOVDD_DGND_M	23
#define ALDO_AVSS_P		24
#define ALDO_AVSS_M		25
#define DLDO_DGND_P		26
#define DLDO_DGND_M		27
#define V_20_MV_P		28
#define V_20_MV_M		29
#define CLR_AINP(x)		x &= AINP_MASK
#define SET_AINP(x, y)	CLR_AINP(x);	\
						x |= ((y) << AINP_OFFSET)
#define CLR_AINM(x)		x &= AINM_MASK
#define SET_AINM(x, y)	CLR_AINM(x);	\
						x |= ((y) << AINM_OFFSET)


//CONFIG REG
#define BIPOLAR_OFFSET	11
#define BIPOLAR_MASK	0xF7FF
#define UNIPOLAR_MODE	0
#define BIPOLAR_MODE	1
#define CLR_BIPOLAR(x)	x &= BIPOLAR_MASK
#define SET_BIPOLAR(x)	CLR_BIPOLAR(x);	\
						x |= (1 << BIPOLAR_OFFSET)
#define SET_UNIPOLAR(x)	CLR_BIPOLAR(x)

#define REFP_BUF_OFFSET	8
#define REFP_BUF_MASK	0xFEFF
#define CLR_REFP_BUF(x)	x &= REFP_BUF_MASK
#define SET_REFP_BUF(x)	CLR_REFP_BUF(x);	\
						x |= (1 << REFP_BUF_OFFSET)

#define REFM_BUF_OFFSET	7
#define REFM_BUF_MASK	0xFF7F
#define CLR_REFM_BUF(x)	x &= REFM_BUF_MASK
#define SET_REFM_BUF(x)	CLR_REFM_BUF(x);	\
						x |= (1 << REFM_BUF_OFFSET)

#define AINP_BUF_OFFSET	6
#define AINP_BUF_MASK	0xFFBF
#define CLR_AINP_BUF(x)	x &= AINP_BUF_MASK
#define SET_AINP_BUF(x)	CLR_AINP_BUF(x);	\
						x |= (1 << AINP_BUF_OFFSET)
#define AINM_BUF_OFFSET	5
#define AINM_BUF_MASK	0xFFDF
#define CLR_AINM_BUF(x)	x &= AINM_BUF_MASK
#define SET_AINM_BUF(x)	CLR_AINM_BUF(x);	\
						x |= (1 << AINM_BUF_OFFSET)

#define REF_SEL_OFFSET	3
#define REF_SEL_MASK	0xFFE7
#define REFIN1_PM		0
#define REFIN2_PM		1
#define INTERNAL_REF	2
#define AVDD_REF		3
#define CLR_REF_SEL(x)	x &= REF_SEL_MASK
#define SET_REF_SEL(x, y)	CLR_REF_SEL(x);	\
							x |= (y << REF_SEL_OFFSET)

#define PGA_OFFSET	0
#define PGA_MASK	0xFFF8
#define PGA_2DOT5V		0
#define PGA_1DOT25V		1
#define PGA_625mV		2
#define PGA_312DOT5mV	3
#define PGA_156DOT25mV	4
#define PGA_78DOT125mV	5
#define PGA_39DOT06mV	6
#define PGA_19DOT53mV	7
#define CLR_PGA(x)	x &= PGA_MASK
#define SET_PGA(x, y)	CLR_PGA(x);	\
						x |= (y << PGA_OFFSET)

//Filter Reg
#define FILTER_TYPE_OFFSET	21
#define FILTER_TYPE_MASK	0x1FFFFF
#define FILTER_SINC4		0
#define FILTER_SINC3		2
#define FILTER_FAST_SINC4	4
#define FILTER_FAST_SINC3	5
#define FILTER_POST			7
#define CLR_FILTER_TYPE(x)	x &= FILTER_TYPE_MASK
#define SET_FILTER_TYPE(x, y)	CLR_FILTER_TYPE(x);	\
								x |= (y << FILTER_TYPE_OFFSET)


#define REJ60_OFFSET		20
#define REJ60_MASK			0xEFFFFF
#define CLR_REJ60(x)		x &= REJ60_MASK
#define SET_REJ60(x)		CLR_REJ60(x);	\
							x |= (1 << REJ60_OFFSET)
#define SET_REJ50(x)		CLR_REJ60(x)

#define POST_FILTER_OFFSET	17
#define POST_FILTER_MASK	0xF1FFFF
#define SPS_27DOT27	2
#define SPS_25		3
#define SPS_20		5
#define SPS16DOT7	6
#define CLR_POST_FILTER(x)	x &= POST_FILTER_MASK
#define SET_POST_FILTER(x, y)	CLR_POST_FILTER(x);	\
								x |= ((y) << POST_FILTER_OFFSET)

#define SINGLE_CYCLE_OFFSET	16
#define SINGLE_CYCLE_MASK	0xFEFFFF
#define CLR_SINGLE_CYCLE(x)	x &= SINGLE_CYCLE_MASK
#define SET_SINGLE_CYCLE(x)	CLR_SINGLE_CYCLE(x);	\
							x |= ((int)1 << SINGLE_CYCLE_OFFSET)

#define FILTER_OUT_DATA_RATE_OFFSET	0
#define FILTER_OUT_DATA_RATE_MASK	0xFFF800
#define CLR_FILTER_OUT_DATA_RATE(x) x &= FILTER_OUT_DATA_RATE_MASK
#define SET_FILTER_OUT_DATA_RATE(x, y)	CLR_FILTER_OUT_DATA_RATE(x);	\
										x |= (y & 0x7FF);


#define AD7124_CRC8_POLYNOMIAL_REPRESENTATION 0x07 /* x8 + x2 + x + 1 */
#define AD7124_DISABLE_CRC 0
#define AD7124_USE_CRC 1


/* Communication Register bits */
#define AD7124_COMM_REG_WEN    (0 << 7)
#define AD7124_COMM_REG_WR     (0 << 6)
#define AD7124_COMM_REG_RD     (1 << 6)
#define AD7124_COMM_REG_RA(x)  ((x) & 0x3F)

/* Status Register bits */
#define AD7124_STATUS_REG_RDY          (1 << 7)
#define AD7124_STATUS_REG_ERROR_FLAG   (1 << 6)
#define AD7124_STATUS_REG_POR_FLAG     (1 << 4)
#define AD7124_STATUS_REG_CH_ACTIVE(x) ((x) & 0xF)

/* ADC_Control Register bits */
#define AD7124_ADC_CTRL_REG_DOUT_RDY_DEL   (1 << 12)
#define AD7124_ADC_CTRL_REG_CONT_READ      (1 << 11)
#define AD7124_ADC_CTRL_REG_DATA_STATUS    (1 << 10)
#define AD7124_ADC_CTRL_REG_CS_EN          (1 << 9)
#define AD7124_ADC_CTRL_REG_REF_EN         (1 << 8)
#define AD7124_ADC_CTRL_REG_POWER_MODE(x)  (((x) & 0x3) << 6)
#define AD7124_ADC_CTRL_REG_MODE(x)        (((x) & 0xF) << 2)
#define AD7124_ADC_CTRL_REG_CLK_SEL(x)     (((x) & 0x3) << 0)

/* IO_Control_1 Register bits */
#define AD7124_IO_CTRL1_REG_GPIO_DAT2     (1 << 23)
#define AD7124_IO_CTRL1_REG_GPIO_DAT1     (1 << 22)
#define AD7124_IO_CTRL1_REG_GPIO_CTRL2    (1 << 19)
#define AD7124_IO_CTRL1_REG_GPIO_CTRL1    (1 << 18)
#define AD7124_IO_CTRL1_REG_PDSW          (1 << 15)
#define AD7124_IO_CTRL1_REG_IOUT1(x)      (((x) & 0x7) << 11)
#define AD7124_IO_CTRL1_REG_IOUT0(x)      (((x) & 0x7) << 8)
#define AD7124_IO_CTRL1_REG_IOUT_CH1(x)   (((x) & 0xF) << 4)
#define AD7124_IO_CTRL1_REG_IOUT_CH0(x)   (((x) & 0xF) << 0)

/* IO_Control_1 AD7124-8 specific bits */
#define AD7124_8_IO_CTRL1_REG_GPIO_DAT4     (1 << 23)
#define AD7124_8_IO_CTRL1_REG_GPIO_DAT3     (1 << 22)
#define AD7124_8_IO_CTRL1_REG_GPIO_DAT2     (1 << 21)
#define AD7124_8_IO_CTRL1_REG_GPIO_DAT1     (1 << 20)
#define AD7124_8_IO_CTRL1_REG_GPIO_CTRL4    (1 << 19)
#define AD7124_8_IO_CTRL1_REG_GPIO_CTRL3    (1 << 18)
#define AD7124_8_IO_CTRL1_REG_GPIO_CTRL2    (1 << 17)
#define AD7124_8_IO_CTRL1_REG_GPIO_CTRL1    (1 << 16)

/* IO_Control_2 Register bits */
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS7   (1 << 15)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS6   (1 << 14)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS5   (1 << 11)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS4   (1 << 10)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS3   (1 << 5)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS2   (1 << 4)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS1   (1 << 1)
#define AD7124_IO_CTRL2_REG_GPIO_VBIAS0   (1 << 0)

/* IO_Control_2 AD7124-8 specific bits */
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS15  (1 << 15)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS14  (1 << 14)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS13  (1 << 13)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS12  (1 << 12)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS11  (1 << 11)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS10  (1 << 10)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS9   (1 << 9)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS8   (1 << 8)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS7   (1 << 7)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS6   (1 << 6)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS5   (1 << 5)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS4   (1 << 4)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS3   (1 << 3)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS2   (1 << 2)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS1   (1 << 1)
#define AD7124_8_IO_CTRL2_REG_GPIO_VBIAS0   (1 << 0)

/* ID Register bits */
#define AD7124_ID_REG_DEVICE_ID(x)   (((x) & 0xF) << 4)
#define AD7124_ID_REG_SILICON_REV(x) (((x) & 0xF) << 0)

/* Error Register bits */
#define AD7124_ERR_REG_LDO_CAP_ERR        (1 << 19)
#define AD7124_ERR_REG_ADC_CAL_ERR        (1 << 18)
#define AD7124_ERR_REG_ADC_CONV_ERR       (1 << 17)
#define AD7124_ERR_REG_ADC_SAT_ERR        (1 << 16)
#define AD7124_ERR_REG_AINP_OV_ERR        (1 << 15)
#define AD7124_ERR_REG_AINP_UV_ERR        (1 << 14)
#define AD7124_ERR_REG_AINM_OV_ERR        (1 << 13)
#define AD7124_ERR_REG_AINM_UV_ERR        (1 << 12)
#define AD7124_ERR_REG_REF_DET_ERR        (1 << 11)
#define AD7124_ERR_REG_DLDO_PSM_ERR       (1 << 9)
#define AD7124_ERR_REG_ALDO_PSM_ERR       (1 << 7)
#define AD7124_ERR_REG_SPI_IGNORE_ERR     (1 << 6)
#define AD7124_ERR_REG_SPI_SLCK_CNT_ERR   (1 << 5)
#define AD7124_ERR_REG_SPI_READ_ERR       (1 << 4)
#define AD7124_ERR_REG_SPI_WRITE_ERR      (1 << 3)
#define AD7124_ERR_REG_SPI_CRC_ERR        (1 << 2)
#define AD7124_ERR_REG_MM_CRC_ERR         (1 << 1)
#define AD7124_ERR_REG_ROM_CRC_ERR        (1 << 0)

/* Error_En Register bits */
#define AD7124_ERREN_REG_MCLK_CNT_EN           (1 << 22)
#define AD7124_ERREN_REG_LDO_CAP_CHK_TEST_EN   (1 << 21)
#define AD7124_ERREN_REG_LDO_CAP_CHK(x)        (((x) & 0x3) << 19)
#define AD7124_ERREN_REG_ADC_CAL_ERR_EN        (1 << 18)
#define AD7124_ERREN_REG_ADC_CONV_ERR_EN       (1 << 17)
#define AD7124_ERREN_REG_ADC_SAT_ERR_EN        (1 << 16)
#define AD7124_ERREN_REG_AINP_OV_ERR_EN        (1 << 15)
#define AD7124_ERREN_REG_AINP_UV_ERR_EN        (1 << 14)
#define AD7124_ERREN_REG_AINM_OV_ERR_EN        (1 << 13)
#define AD7124_ERREN_REG_AINM_UV_ERR_EN        (1 << 12)
#define AD7124_ERREN_REG_REF_DET_ERR_EN        (1 << 11)
#define AD7124_ERREN_REG_DLDO_PSM_TRIP_TEST_EN (1 << 10)
#define AD7124_ERREN_REG_DLDO_PSM_ERR_ERR      (1 << 9)
#define AD7124_ERREN_REG_ALDO_PSM_TRIP_TEST_EN (1 << 8)
#define AD7124_ERREN_REG_ALDO_PSM_ERR_EN       (1 << 7)
#define AD7124_ERREN_REG_SPI_IGNORE_ERR_EN     (1 << 6)
#define AD7124_ERREN_REG_SPI_SCLK_CNT_ERR_EN   (1 << 5)
#define AD7124_ERREN_REG_SPI_READ_ERR_EN       (1 << 4)
#define AD7124_ERREN_REG_SPI_WRITE_ERR_EN      (1 << 3)
#define AD7124_ERREN_REG_SPI_CRC_ERR_EN        (1 << 2)
#define AD7124_ERREN_REG_MM_CRC_ERR_EN         (1 << 1)
#define AD7124_ERREN_REG_ROM_CRC_ERR_EN        (1 << 0)

/* Channel Registers 0-15 bits */
#define AD7124_CH_MAP_REG_CH_ENABLE    (1 << 15)
#define AD7124_CH_MAP_REG_SETUP(x)     (((x) & 0x7) << 12)
#define AD7124_CH_MAP_REG_AINP(x)      (((x) & 0x1F) << 5)
#define AD7124_CH_MAP_REG_AINM(x)      (((x) & 0x1F) << 0)

/* Configuration Registers 0-7 bits */
#define AD7124_CFG_REG_BIPOLAR     (1 << 11)
#define AD7124_CFG_REG_BURNOUT(x)  (((x) & 0x3) << 9)
#define AD7124_CFG_REG_REF_BUFP    (1 << 8)
#define AD7124_CFG_REG_REF_BUFM    (1 << 7)
#define AD7124_CFG_REG_AIN_BUFP    (1 << 6)
#define AD7124_CFG_REG_AINN_BUFM   (1 << 5)
#define AD7124_CFG_REG_REF_SEL(x)  ((x) & 0x3) << 3
#define AD7124_CFG_REG_PGA(x)      (((x) & 0x7) << 0)

/* Filter Register 0-7 bits */
#define AD7124_FILT_REG_FILTER(x)         (((x) & 0x7) << 21)
#define AD7124_FILT_REG_REJ60             (1 << 20)
#define AD7124_FILT_REG_POST_FILTER(x)    (((x) & 0x7) << 17)
#define AD7124_FILT_REG_SINGLE_CYCLE      (1 << 16)
#define AD7124_FILT_REG_FS(x)             (((x) & 0x7FF) << 0)

typedef struct _AD7124_DEV
{	uint16_t 		rdyPollCnt;
	uint16_t 		useCrc;
	uint8_t 		chkRdy;
	ADC_REG_INFO 	*adcRegInfo;
	//uint8_t			gainVal[8];	//max 8 config
	//uint8_t			bipolarSel[8];	//max 8 config
	uint8_t			errorOccurred;
	uint8_t			differential;
	int				prevEnableChannel;
}AD7124_DEV;

extern int openAdcSpi(uint32_t *chipId, uint8_t differential);
extern int closeAdcSpi(void);

#if SINGLE_CONVERSION_SINGLE_CHANNEL_EN == 1
extern float ad7124ReadChannelValue(uint8_t channel);
#elif SINGLE_CONVERSION_SEVERAL_CHANNELS_EN == 1
extern int ad7124ReadChannelValue(uint8_t channel, float *aiVal);
#endif

#endif
