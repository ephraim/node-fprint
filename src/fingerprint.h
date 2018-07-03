#include <nan.h>
#include <libfprint/fprint.h>
#include <string>

extern int initalized;

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

int fromFPDev(struct fp_dev *dev);
struct fp_dev* toFPDev(int value);

std::string toString(unsigned char* buffer, unsigned long size);
unsigned char* fromString(std::string hex, unsigned long *size);
