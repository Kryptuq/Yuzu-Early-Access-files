#include <stdio.h>
#include <openssl/evp.h>

int  main(int argc, char *argv[])
 {
 EVP_MD_CTX *mdctx;
 const EVP_MD *md;
 char mess1[] = "eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZCI6MSwiaXNzIjoiYXJ1bi5jb20iLCJ0aW1lX3N0ciI6Ijg6MThwbSAyNCBOb3YgMjAxNyIsIndoZXJlIjoiYWlycG9ydCJ9";
 unsigned char md_value[EVP_MAX_MD_SIZE];
 int md_len, i;

 //OpenSSL_add_all_digests();

 if(!argv[1]) {
        printf("Usage: mdtest digestname\n");
        exit(1);
 }

 md = EVP_sha256();

 if(!md) {
        printf("Unknown message digest %s\n", argv[1]);
        exit(1);
 }

 mdctx = EVP_MD_CTX_create();
 EVP_DigestInit_ex(mdctx, md, NULL);
 EVP_DigestUpdate(mdctx, mess1, strlen(mess1));
 EVP_DigestFinal_ex(mdctx, md_value, &md_len);
 EVP_MD_CTX_destroy(mdctx);

 printf("Dig: %s\n", md_value);
 printf("Dig: %d\n", md_len);

 printf("Digest is: ");
 for(i = 0; i < md_len; i++)
        printf("%02x", md_value[i]);
 printf("\n");

 d2i_ECDSA_SIG(NULL, (const unsigned char **)&md_value[0], md_len);

 /* Call this once before exit. */
 EVP_cleanup();
 exit(0);
 }
