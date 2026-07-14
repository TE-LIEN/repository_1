
#include "ksdk_mbedtls_config.h"


#if defined(MBEDTLS_SHA256_ALT)
#include "mbedtls/sha256.h"
//#include "xilrsa.h"

__attribute__ ((weak)) void mbedtls_sha256_init( mbedtls_sha256_context *ctx )
{
    SHA256_VALIDATE( ctx != NULL );

    memset( ctx, 0, sizeof( mbedtls_sha256_context ) );
}

void mbedtls_sha256_free( mbedtls_sha256_context *ctx )
{
    if( ctx == NULL )
        return;

    mbedtls_platform_zeroize( ctx, sizeof( mbedtls_sha256_context ) );
}

void mbedtls_sha256_clone( mbedtls_sha256_context *dst,
                           const mbedtls_sha256_context *src )
{
    SHA256_VALIDATE( dst != NULL );
    SHA256_VALIDATE( src != NULL );

    *dst = *src;
}

int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 )
{	sha2_context *sha2Ctx;
	sha2Ctx = (sha2_context *)&ctx->state[0];

	ctx->total[0] = 0;
	ctx->total[1] = 0;
	if( is224 == 0 )
	{	sha2_starts(&sha2Ctx);
	}
	else
	{	ctx->state[0] = 0xC1059ED8;
    	ctx->state[1] = 0x367CD507;
    	ctx->state[2] = 0x3070DD17;
    	ctx->state[3] = 0xF70E5939;
    	ctx->state[4] = 0xFFC00B31;
    	ctx->state[5] = 0x68581511;
    	ctx->state[6] = 0x64F98FA7;
    	ctx->state[7] = 0xBEFA4FA4;
	}
	ctx->is224 = is224;
	return( 0 );
}

void mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224)
{
    (void)mbedtls_sha256_starts_ret(ctx, is224);
}

#endif
