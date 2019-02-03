#ifndef EXCEPTION_H
#define EXCEPTION_H

typedef struct{
	char * ptr;
}INVALID_DRIVE;
INVALID_DRIVE idr = {"Cannot read disk. Please check."};

typedef struct{
	char * ptr;
}INVALID_READ;
INVALID_READ rdr = {"Disk read checkout failure. Please contact administrator."};

#endif