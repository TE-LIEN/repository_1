./objects/psa_crypto_client.o: library\psa_crypto_client.c \
  library\common.h include\mbedtls\build_info.h \
  port\nuvoton\m467_mbedtls_config.h \
  ..\FreeRTOS\Source\include\FreeRTOS.h \
  ..\FreeRTOS\Source\FreeRTOSConfig.h \
  ..\FreeRTOS\Source\include\projdefs.h \
  ..\FreeRTOS\Source\include\portable.h \
  ..\FreeRTOS\Source\include\deprecated_definitions.h \
  ..\FreeRTOS\Source\portable\GCC\ARM_CM4F\portmacro.h \
  ..\FreeRTOS\Source\include\mpu_wrappers.h \
  include\mbedtls\check_config.h include\psa\crypto.h \
  include\psa\crypto_platform.h include\mbedtls\private_access.h \
  include\mbedtls\config_psa.h include\psa\crypto_types.h \
  include\psa\crypto_values.h include\psa\crypto_sizes.h \
  include\psa\crypto_struct.h include\mbedtls\cmac.h \
  include\mbedtls\cipher.h include\mbedtls\platform_util.h \
  include\mbedtls\gcm.h include\mbedtls\ccm.h \
  include\mbedtls\chachapoly.h include\mbedtls\poly1305.h \
  include\mbedtls\chacha20.h \
  include\psa\crypto_driver_contexts_primitives.h \
  include\psa\crypto_driver_common.h \
  include\psa\crypto_builtin_primitives.h include\mbedtls\md5.h \
  include\mbedtls\ripemd160.h include\mbedtls\sha1.h \
  port\nuvoton\sha1_alt.h port\nuvoton\sha_alt_hw.h \
  port\nuvoton\sha1_alt_sw.h include\mbedtls\sha256.h \
  port\nuvoton\sha256_alt.h port\nuvoton\sha256_alt_sw.h \
  include\mbedtls\sha512.h port\nuvoton\sha512_alt.h \
  port\nuvoton\sha512_alt_sw.h \
  include\psa\crypto_driver_contexts_composites.h \
  include\psa\crypto_builtin_composites.h include\psa\crypto_extra.h \
  include\psa\crypto_compat.h include\mbedtls\ecp.h \
  include\mbedtls\bignum.h port\nuvoton\ecp_alt.h port\incConf.h
