# 源码文件与关键函数定位

行号对应未修改的锁定上游快照；板级 overlay 应以符号/实际文件为准。这是导航索引，不是完整 C 语法分析器。

## Makefile

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/Makefile)；SHA256 `e038758c8dea47e27465be92ed8d51a125f7cb0fec576a9165a31ff485765188`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## tools/config.mk

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/tools/config.mk)；SHA256 `99c623968430cff13cef506a522a9bc08cab29576e4eb6cb440dd29c090e2aae`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## arch.mk

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/arch.mk)；SHA256 `fb7794c6dfd93fb703b1dbc246cf0e331c218da9c5c9483cfa634a9636f90781`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## options.mk

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/options.mk)；SHA256 `7ac6b129433cda69bdf5b8249ab1f473419a01fbbd52e00ba8af0f86ab5b072f`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## include/target.h.in

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/include/target.h.in)；SHA256 `2b3b059f88fafa4aeab939e3cea4208bac9dc98b33b4350911bdb0d49d4962c3`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## hal/stm32f4.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/hal/stm32f4.c)；SHA256 `9fa6c951809e8b2c523a64800031e4f884c25106e487a78861321a4c74988492`。

- L139：`flash_set_waitstates`
- L150：`mass_erase`
- L159：`flash_erase_sector`
- L170：`clear_errors`
- L175：`hal_flash_write`
- L192：`hal_flash_unlock`
- L208：`hal_cache_invalidate`
- L223：`hal_flash_lock`
- L233：`hal_flash_erase`
- L264：`clock_pll_off`
- L283：`clock_pll_on`
- L362：`hal_init`
- L371：`hal_prepare_boot`

## hal/hal.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/hal/hal.c)；SHA256 `b938534c645dd572de165b7d3b133b812bf15271ed1f380a2c487ed83eb3abe3`。

- L44：`hal_flash_test`
- L88：`hal_flash_test_write_once`
- L134：`hal_flash_test_align`
- L185：`hal_flash_test_unaligned_src`
- L227：`hal_flash_test_dualbank`

## hal/stm32f4.ld

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/hal/stm32f4.ld)；SHA256 `4c5f53b81c141c2dcaaa0681bfcc96f0e1eb0fceaf6da0b2c72a8c9ac2a51ee7`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## include/hal.h

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/include/hal.h)；SHA256 `8f684333a828865943d30ec6f10101f3d57dbe9b4785639a1cd74b7fa6c40097`。

- L263：`hal_dice_update_cdi`
- L274：`hal_dice_sign_hash`

## src/boot_arm.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/src/boot_arm.c)；SHA256 `42e8b97f82ca4d4c135d06ba1e57a691d54769a9d155be4a57542ad627a80631`。

- L73：`mpu_setaddr`
- L79：`mpu_setattr`
- L85：`mpu_on`
- L116：`mpusize`
- L132：`mpu_init`
- L183：`mpu_off`
- L225：`isr_reset`
- L239：`isr_reset_c`
- L246：`isr_reset`
- L261：`isr_reset_c`
- L263：`isr_reset`
- L392：`HardFault_HandlerC`
- L455：`isr_fault`
- L475：`isr_fault`
- L482：`isr_empty`
- L532：`do_boot`
- L1105：`arch_reboot`

## src/loader.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/src/loader.c)；SHA256 `21de8ae57123664cb1ac461d8c8039e50ee9fbb5776191f2f1568025b3e54d75`。

- L70：`wolfBoot_fipsCb`
- L97：`main`
- L108：`loader_main`
- L110：`main`

## src/image.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/src/image.c)；SHA256 `f1d4b0b21c3c5a3f8ef6a759fc44109428ebdf1647ad43e7e9c921ab0b631c64`。

- L161：`wolfBoot_verify_signature_tpm`
- L211：`wolfBoot_verify_signature_ed25519`
- L235：`wolfBoot_verify_signature_ed448`
- L283：`wolfBoot_verify_signature_ecc`
- L449：`DecodeAsn1Tag`
- L485：`RsaDecodeSignature`
- L523：`wolfBoot_verify_signature_rsa_common`
- L707：`wolfBoot_verify_signature_lms`
- L776：`wolfBoot_verify_signature_xmss`
- L836：`wolfBoot_verify_signature_ml_dsa`
- L1028：`get_header_ext`
- L1041：`wolfBoot_get_header`
- L1132：`wolfBoot_invalidate_hdr_cache`
- L1137：`get_header_ext`
- L1146：`get_header_ext`
- L1166：`header_sha256`
- L1209：`image_sha256`
- L1285：`key_sha256`
- L1307：`header_sha384`
- L1353：`image_sha384`
- L1405：`key_sha384`
- L1428：`header_sha3_384`
- L1474：`image_sha3_384`
- L1522：`key_sha3_384`
- L1548：`im2n`
- L1567：`wolfBoot_image_size`
- L1585：`wolfBoot_open_image_address`
- L1659：`wolfBoot_get_dts_size`
- L1672：`wolfBoot_hash_buffer`
- L1738：`wolfBoot_verify_dts_digest`
- L1770：`wolfBoot_open_image`
- L1847：`wolfBoot_open_image_external`
- L1883：`wolfBoot_open_self`
- L1909：`wolfBoot_open_self_address`
- L1961：`wolfBoot_verify_integrity`
- L2005：`copy_flash_buffered`
- L2087：`read_flash_fwimage`
- L2125：`read_flash_addr`
- L2153：`update_hash_flash_fwimg`
- L2196：`update_hash_flash_addr`
- L2231：`wolfBoot_check_flash_image_elf`
- L2489：`wolfBoot_load_flash_image_elf`
- L2647：`wolfBoot_verify_authenticity`
- L2653：`wolfBoot_verify_authenticity`
- L2971：`keyslot_CT_hint_matches`
- L2993：`keyslot_id_by_sha`

## src/libwolfboot.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/src/libwolfboot.c)；SHA256 `961396c895cb36426a85a572ae483fab9e64d571279f361e2cbc678cad6136ae`。

- L75：`encrypt_key_is_erased`
- L96：`encrypt_key_is_valid`
- L123：`wolfBoot_initialize_encryption`
- L207：`wb_reverse_word32`
- L290：`nvm_cache_scrub`
- L298：`get_base_offset`
- L306：`nvm_select_fresh_sector`
- L404：`trailer_write`
- L455：`partition_magic_write`
- L509：`set_trailer_at`
- L516：`set_partition_magic`
- L592：`set_trailer_at`
- L635：`set_partition_magic`
- L693：`set_partition_state`
- L711：`set_update_sector_flags`
- L739：`wolfBoot_set_partition_state`
- L764：`wolfBoot_set_update_sector_flag`
- L796：`wolfBoot_get_partition_state`
- L823：`wolfBoot_get_update_sector_flag`
- L849：`wolfBoot_erase_partition`
- L893：`wolfBoot_update_trigger`
- L964：`wolfBoot_success`
- L1048：`diag_crc32`
- L1061：`diag_read`
- L1071：`diag_read_header`
- L1085：`diag_read_record`
- L1097：`diag_sector_count`
- L1111：`diag_scan`
- L1137：`diag_ordered`
- L1150：`diag_unlock`
- L1159：`diag_lock`
- L1168：`diag_erase`
- L1180：`diag_write`
- L1191：`diag_write_header`
- L1206：`diag_max_seq`
- L1221：`diag_slot_is_erased`
- L1236：`wolfBoot_record_failure`
- L1296：`wolfBoot_get_failure_count`
- L1308：`wolfBoot_get_failure`
- L1334：`wolfBoot_clear_failures`
- L1358：`wolfBoot_find_header`
- L1450：`im2n`
- L1471：`im2ns`
- L1501：`wolfBoot_get_delta_info`
- L1543：`decrypt_header`
- L1563：`dec_hdr_clear`
- L1581：`wolfBoot_get_blob_version`
- L1616：`wolfBoot_get_blob_type`
- L1652：`wolfBoot_get_blob_diffbase_version`
- L1739：`wolfBoot_get_self_version`
- L1765：`wolfBoot_get_image_version`
- L1785：`wolfBoot_get_diffbase_version`
- L1804：`wolfBoot_get_image_type`
- L1832：`wolfBoot_dualboot_candidate`
- L1878：`wolfBoot_current_firmware_version`
- L1882：`wolfBoot_update_firmware_version`
- L1887：`wolfBoot_dualboot_candidate_addr`
- L1924：`wolfBoot_fallback_is_possible`
- L1968：`wolfBoot_enable_fallback_iv`
- L1982：`wolfBoot_force_fallback_iv`
- L1989：`wolfBoot_crypto_set_iv`
- L2009：`hal_set_key`
- L2111：`wolfBoot_set_encrypt_key`
- L2130：`wolfBoot_get_encrypt_key`
- L2168：`wolfBoot_erase_encrypt_key`
- L2199：`chacha_init`
- L2258：`aes_init`
- L2352：`aes_set_iv`
- L2397：`pkcs11_crypto_init`
- L2498：`pkcs11_crypto_set_iv`
- L2552：`pkcs11_crypto_encrypt`
- L2586：`pkcs11_crypto_decrypt`
- L2624：`pkcs11_pin_wipe`
- L2635：`pkcs11_crypto_deinit`
- L2662：`part_address`
- L2711：`ext_flash_encrypt_write`
- L2872：`ext_flash_decrypt_read`
- L2994：`wolfBoot_ram_decrypt`
- L3111：`wolfBoot_nsc_success`
- L3117：`wolfBoot_nsc_update_trigger`
- L3123：`wolfBoot_nsc_get_image_version`
- L3129：`wolfBoot_nsc_get_partition_state`
- L3137：`wolfBoot_nsc_erase_update`
- L3159：`wolfBoot_nsc_write_update`

## src/update_flash.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/src/update_flash.c)；SHA256 `47053e961a43b614cfd1fd7b0f51eefb505b4146e0d2439cf7111bf6fbd1efe6`。

- L39：`wolfBoot_zeroize`
- L48：`wolfBoot_local_constant_compare`
- L96：`wolfBoot_erase_bootloader`
- L114：`wolfBoot_update_self_header`
- L180：`wolfBoot_self_update`
- L224：`wolfBoot_record_verify_failure`
- L228：`wolfBoot_check_self_update`
- L278：`wolfBoot_copy_sector`
- L394：`wolfBoot_backup_last_boot_sector`
- L488：`wolfBoot_swap_and_final_erase`
- L661：`wb_delta_im2n`
- L673：`wolfBoot_delta_update`
- L940：`wolfBoot_get_total_size`
- L957：`wolfBoot_hdr_le32`
- L967：`wolfBoot_swap_update_size`
- L1030：`wolfBoot_record_verify_failure`
- L1046：`wolfBoot_update`
- L1556：`wolfBoot_unlock_disk`
- L1677：`wolfBoot_prepare_secure_handoff`
- L1697：`wolfBoot_start`

## include/image.h

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/include/image.h)；SHA256 `86e7828ff2cab8e725114a7220e6f8264e0f95498740f758a49cd82f11d094bc`。

- L1711：`image_CT_compare`
- L1713：`wolfBoot_hardened_CT_compare`
- L1719：`wolfBoot_verify_dts_digest`
- L1729：`wolfBoot_open_self_address`
- L1740：`wolfBoot_load_flash_image_elf`
- L1796：`wb_flash_erase`
- L1805：`wb_flash_write`
- L1814：`wb_flash_write_verify_word`
- L1874：`ramboot_region_overlap`

## include/wolfboot/wolfboot.h

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/include/wolfboot/wolfboot.h)；SHA256 `5701663f451c37b28b63d370ea65e4406281d08cec4220005c653373e41dc2a6`。

- L690：`wolfBoot_get_blob_partition_id`
- L746：`wolfBoot_record_failure`

## include/keystore.h

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/include/keystore.h)；SHA256 `6732b4a42347fbcdcdc3e1445750476bba48b74d232790a8a693c1add2d5bcd3`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## include/user_settings.h

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/include/user_settings.h)；SHA256 `557b67eddf6adca81d21badc63ffa84767ead456d8c5245202f660e98c47f30c`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## src/string.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/src/string.c)；SHA256 `528486ecff8aad7ea2688484a6c5723f8fb6e9c110c5f54bac1b47a5cc7b2e8a`。

- L73：`islower`
- L78：`isupper`
- L83：`tolower`
- L88：`toupper`
- L93：`isalpha`
- L145：`strcmp`
- L157：`strcasecmp`
- L168：`strncasecmp`
- L197：`strncmp`
- L244：`memcmp`
- L370：`__aeabi_memcpy`
- L374：`__aeabi_memcpy4`
- L378：`__aeabi_memcpy8`
- L382：`__aeabi_memmove`
- L387：`__aeabi_memset`
- L391：`__aeabi_memset4`
- L395：`__aeabi_memset8`
- L399：`__aeabi_memclr`
- L403：`__aeabi_memclr4`
- L407：`__aeabi_memclr8`
- L423：`uart_writenum_emit`
- L433：`uart_writenum`
- L470：`uart_writenum_ll`
- L498：`uart_vprintf`
- L741：`uart_printf`

## test-app/Makefile

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/test-app/Makefile)；SHA256 `ad4c142b3792abb843605899fd847cbb771356392af5418da8fe488e05addd0b`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## test-app/ARM.ld

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/test-app/ARM.ld)；SHA256 `3c24f1020e4be1e56e7381512aad4084106e6cc9fd2307daffb2c30c446d6711`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## test-app/app_stm32f4.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/test-app/app_stm32f4.c)；SHA256 `cbef9cd24f70605e077004d484e8549c10c4a5f93347dd5d4c0f9e87ddbefc78`。

- L122：`arch_reboot`
- L131：`uart_write`
- L140：`uart_pins_setup`
- L157：`uart_setup`
- L212：`ack`
- L222：`check`
- L236：`main`

## test-app/system.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/test-app/system.c)；SHA256 `bb0d68faa4e17bfe129c339c86498125a5ac0057afbbca6795da2ad6806ab488`。

- L88：`flash_set_waitstates`
- L94：`clock_config`

## test-app/timer.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/test-app/timer.c)；SHA256 `88e9fcc46be0a0daa326c098b5f707d5dfa4cbe5abcd734f1db3fbaf24701cd7`。

- L77：`pwm_init`
- L113：`timer_init`
- L151：`isr_tim2`
- L167：`isr_tim2`

## test-app/led.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/test-app/led.c)；SHA256 `0b642094badda28e7acb3b3ff07e2003c5be22109c04689fc9d0cce14a24440f`。

- L43：`led_pwm_setup`
- L61：`boot_led_on`
- L93：`boot_led_on`
- L105：`boot_led_off`
- L131：`boot_led_on`
- L161：`boot_led_on`
- L173：`boot_led_off`
- L201：`boot_led_on`
- L212：`led_on`
- L223：`led_off`
- L227：`boot_led_off`
- L238：`boot_led_on`
- L252：`boot_led_off`

## tools/keytools/keygen.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/tools/keytools/keygen.c)；SHA256 `a63571f008d883dc51dec244b0240a8ab7040b3bbfc9569e96548855e7a7dea8`。

- L114：`keygen_die`
- L270：`usage`
- L279：`fwritekey`
- L404：`export_pubkey_file`
- L428：`get_pubkey_size`
- L501：`keystore_add`
- L567：`keygen_rsa`
- L637：`keygen_ecc`
- L763：`keygen_ed25519`
- L826：`keygen_ed448`
- L891：`keygen_lms`
- L1013：`keygen_xmss`
- L1131：`keygen_ml_dsa`
- L1343：`key_gen_check`
- L1370：`key_generate`
- L1442：`key_import`
- L1526：`parse_id`
- L1550：`main`

## tools/keytools/sign.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/tools/keytools/sign.c)；SHA256 `3d0f512983a4e6784dd40eb0d55b8e77c78964e7c40b3eaf348f0cd0689553cc`。

- L61：`fp_truncate`
- L223：`header_store_u16_le`
- L229：`header_store_u32_le`
- L237：`header_store_u64_le`
- L246：`header_append_u32`
- L251：`header_append_u16`
- L259：`header_append_tag`
- L279：`header_append_tag_u16`
- L288：`header_append_tag_u32`
- L297：`header_append_tag_u64`
- L338：`free_key`
- L422：`header_append_limit`
- L427：`zero_and_free`
- L437：`sign_tool_find_header`
- L491：`load_key_ecc`
- L599：`load_key_rsa`
- L1131：`sign_digest`
- L1315：`header_size_align_8`
- L1322：`header_size_align_4`
- L1329：`header_size_append_tag`
- L1334：`header_digest_size`
- L1352：`dts_hash_file`
- L1498：`header_required_size`
- L1583：`make_header_ex`
- L2590：`make_header`
- L2597：`make_header_delta`
- L2609：`make_hybrid_header`
- L2617：`base_diff`
- L2960：`arg2num`
- L2996：`extract_pubkey_from_der`
- L3147：`set_signature_sizes`
- L3363：`main`

## tools/keytools/Makefile

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/tools/keytools/Makefile)；SHA256 `ab80c95766743fa51f70625721628b8ccb676ed1f5b07d61f8211880c0b187cd`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## tools/bin-assemble/bin-assemble.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/tools/bin-assemble/bin-assemble.c)；SHA256 `a0b1ea16d124aa0318d2377f2a280c325fef26e5b43cdfe49c820d8badf8b497`。

- L46：`usage`
- L61：`binentry_address_compare`
- L85：`main`

## config/openocd/openocd_stm32f4.cfg

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/config/openocd/openocd_stm32f4.cfg)；SHA256 `0293dd2ae080f720e07df3e5cc462b3e32e3deb8f8d03f2cbb741c0af43a8a2b`。

该文件为构建/声明/配置文件；职责与阅读重点见第 6 篇。

## tools/unit-tests/unit-update-flash.c

[固定 commit 中的文件](https://github.com/wolfSSL/wolfBoot/blob/0a3b0f7184a7a3d378e2a63b3d1e9080d03c3f8f/tools/unit-tests/unit-update-flash.c)；SHA256 `68afee4b4ba6af71fb7dad229807e82b4c0e8b150fad00a574a81ecd423aa558`。

- L63：`add_payload_type`
- L66：`host_to_img_u32`
- L78：`host_to_img_u16`
- L92：`update_part_write`
- L97：`ext_flash_write_le16`
- L105：`ext_flash_write_le32`
- L120：`unit_test_wb_patch_init`
- L132：`unit_test_wb_patch`
- L147：`wolfBoot_get_encrypt_key`
- L161：`wolfBoot_set_encrypt_key`
- L169：`wolfBoot_erase_encrypt_key`
- L235：`do_boot`
- L251：`hal_flash_protect`
- L260：`wolfBoot_hook_boot`
- L267：`reset_mock_stats`
- L302：`clear_erase_stats`
- L313：`prepare_flash`
- L331：`cleanup_flash`
- L341：`add_payload_encrypted`
- L345：`add_payload`
- L351：`add_payload_type`
- L442：`arch_reboot`
- L540：`build_image_buffer`
- L602：`add_payload_encrypted`
- L1122：`update_read_plain`
- L1134：`check_update_image`
- L1142：`resume_setup_versions`
- L1154：`resume_setup`
- L1159：`resume_verify`
- L1201：`cut_after_sector0_backup_erase`
- L1280：`resume_with_version`
- L1313：`swap_copy_set_u32`
- L1352：`resume_with_bad_swap_field`
- L1403：`roundtrip_run`
- L1892：`prepare_inverse_delta_update`
- L2458：`main`
