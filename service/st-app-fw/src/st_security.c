#include "st_security.h"

unsigned char iv[16] = {0};
unsigned char salt[32] = {0};

// Generate random number
int gen_random(unsigned char* random, unsigned int random_len)
{
  int ret = 0;
   unsigned char mac[6] = { 0 };

  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;
  mbedtls_entropy_init( &entropy );

   if (!oc_get_mac_addr(mac)) {
    st_print_log("[ST_SEC] oc_get_mac_addr failed!\n");
    goto cleanup;
  }

  ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)mac, strlen(mac) );
  if(ret != 0)
  {
    st_print_log("[ST_SEC]failed in mbedtls_ctr_drbg_seed: %d\n", ret );
    goto cleanup;
  }

  mbedtls_ctr_drbg_set_prediction_resistance( &ctr_drbg, MBEDTLS_CTR_DRBG_PR_OFF );

  ret = mbedtls_ctr_drbg_random(&ctr_drbg, random, random_len);
  if(ret != 0)
  {
    st_print_log("[ST_SEC]failed in mbedtls_ctr_drbg_random: %d\n", ret);
    goto cleanup;
  }

  ret = 0;
  cleanup:

  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  return ret;
}

int pbkdf2(const char *password, unsigned char* key,unsigned char * salt)
{
  int ret = 0;

  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t *info;
  mbedtls_md_init(&ctx);

  info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if(info == NULL)
  {
    st_print_log("[ST_SEC]failed in mbedtls_md_info_from_type\n");
    goto cleanup;
  }

  ret = mbedtls_md_setup(&ctx, info, 1);
  if(ret != 0)
  {
    st_print_log("[ST_SEC]failed in mbedtls_md_setup: %d", ret);
    goto cleanup;
  }

  ret = mbedtls_pkcs5_pbkdf2_hmac(&ctx, password, strlen(password), salt, 32, 1000, 32, key);
  if(ret != 0)
  {
    st_print_log("[ST_SEC]failed in mbedtls_pkcs5_pbkdf2_hmac: %d", ret);
    goto cleanup;
  }

  ret = 0;
  cleanup:
  mbedtls_md_free(&ctx);

  return ret;
}

int aes_encrypt(const unsigned char* key, unsigned char* iv, const unsigned char* data, const unsigned int data_len, unsigned char* encrypted_data, unsigned int* encrypted_data_len)
  {
  int ret = 0;

  unsigned char temp_iv[16] = {0};
  unsigned char* padded_data = NULL;
  unsigned int padded_data_len;
  unsigned int padding_len;

  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);

  // Use temp_iv because mbedtls_aes_crypt_cbc change iv param
  memcpy(temp_iv, iv, 16);

  // Set padding
  if(data_len % 16 == 0){
    padding_len = 0;
  }else{
    padding_len = 16 - (data_len % 16);
  }

  padded_data_len = data_len + padding_len;

  padded_data = (unsigned char*)malloc(padded_data_len);
  // Set PKCS7 padding
  memset(padded_data, padding_len, padded_data_len);
  memcpy(padded_data, data, data_len);

  // Set 256 bit (32 byte) key
  mbedtls_aes_setkey_enc(&aes_ctx, key, 256);
  ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, padded_data_len, temp_iv, padded_data, encrypted_data);
  if(ret != 0)
  {
    printf("failed in mbedtls_aes_crypt_cbc: %d\n", ret);
    goto cleanup;
  }

  *encrypted_data_len = padded_data_len;

  st_print_log("[ST_SEC] encrypted_data_len %d \n",encrypted_data_len);

  ret = 0;

 cleanup:
  mbedtls_aes_free(&aes_ctx);

  return ret;
}

int aes_decrypt(const unsigned char* key, const unsigned char* iv, unsigned char* encrypted_data, unsigned int encrypted_data_len, unsigned char* decrypted_data, unsigned int* decrypted_data_len)
{
  int ret = 0;
  unsigned char i = 0;
  unsigned char padding_len = 0;
  int is_padding = 1;

  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);

  // Set 256 bit (32 byte) key
  mbedtls_aes_setkey_dec(&aes_ctx, key, 256);
  ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, encrypted_data_len, iv, encrypted_data, decrypted_data);
  if(ret != 0)
  {
    st_print_log("[ST_SEC]failed in mbedtls_aes_crypt_cbc: %d\n", ret);
    goto cleanup;
  }

  padding_len = decrypted_data[encrypted_data_len - 1];
  st_print_log("[ST_SEC]padding len = 0x%02x\n", padding_len);

  // Checking PKCS7 padding
  for(i = 1; i<= padding_len; i++){
    if(padding_len != decrypted_data[encrypted_data_len - i]){
      is_padding = 0;
      break;
    }
  }

  if(is_padding == 1){
    // Remove padding
    *decrypted_data_len = encrypted_data_len - padding_len;
  }else{
    // No padding
    *decrypted_data_len = encrypted_data_len;
  }

  ret = 0;

 cleanup:
  mbedtls_aes_free(&aes_ctx);

  return ret;
}

int st_security_encrypt(const unsigned char* data, const unsigned int data_len, unsigned char* encrypted_data, unsigned int* encrypted_data_len)
{
  unsigned char key[32] = {0};
  unsigned char mac[6] = { 0 };
  int ret = 0;

  //Check if already exists
  if(salt == NULL && iv == NULL){

    ret = gen_random(salt, 32);
    if(ret != 0)
    {
     st_print_log("[ST_SEC]failed in gen_random: %d\n", ret);
     return -1;
    }

    // Generate 32 byte key
    pbkdf2(mac, key,salt);

    // Use random 16 byte for iv
    ret = gen_random(iv, 16);
    if(ret != 0)
    {
      st_print_log("[ST_SEC]failed in gen_random: %d\n", ret);
      return -1;
    }
      //Dumping security info
    st_store_t *store_info = st_store_get_info();
    if (oc_string_len(store_info->securityinfo.salt) > 0){
      oc_free_string(&store_info->securityinfo.salt);
    oc_new_string(&store_info->securityinfo.salt, salt,strlen(salt));}
    if (oc_string_len(store_info->securityinfo.iv) > 0){
      oc_free_string(&store_info->securityinfo.iv);
      oc_new_string(&store_info->securityinfo.iv, iv,strlen(iv));}
    st_store_dump_async();
  }

  if (!oc_get_mac_addr(mac)) {
  st_print_log("[ST_ES] oc_get_mac_addr failed!\n");
  return -1;
  }

  // Generate 32 byte key
  pbkdf2(mac, key,salt);

  ret = aes_encrypt(key, iv, data, data_len, encrypted_data, encrypted_data_len);

  return ret;
}

int st_security_decrypt(unsigned char *salt,unsigned char *iv, unsigned char* encrypted_data, unsigned int encrypted_data_len, unsigned char* decrypted_data, unsigned int* decrypted_data_len)
{
  unsigned char key[32] = {0};
  unsigned char mac[6] = { 0 };
  int ret = 0;

  if (!oc_get_mac_addr(mac)) {
    st_print_log("[ST_ES] oc_get_mac_addr failed!\n");
    return -1;
  }

  // Generate 32 byte key
  pbkdf2(mac, key,salt);

  ret = aes_decrypt(key, iv, encrypted_data, encrypted_data_len,decrypted_data, decrypted_data_len);
  return ret;
}
