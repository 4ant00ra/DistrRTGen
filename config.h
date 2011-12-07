#ifndef __CONFIG_H_
#define __CONFIG_H_

#define SERVER "localhost"
#define PORT 35715
#define DATA_CHUNK_SIZE 16*100

#ifndef WIN32
#define Sleep(num) sleep(num / 1000)
#endif
#endif
