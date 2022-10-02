#include "src_client.h"




int main()
{
    Client client;
    client.start();

    /*const unsigned char ibuf[] = "compute sha1";
    unsigned char obuf[64];

    int size = strlen(reinterpret_cast<const char*>(ibuf));

    SHA512(ibuf, size, obuf);
    for (int i = 0; i < 64; i++) {
        printf("%02x ", obuf[i]);
    }
    printf("\n");*/

    system("pause");
    return 0;
}

