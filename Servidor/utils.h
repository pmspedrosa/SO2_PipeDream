#pragma once
#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#define MAX 256


TCHAR comandosParaString(TCHAR** arraycmd, int numargs);
TCHAR** divideString(TCHAR* comando, const TCHAR* delim, unsigned int* tam);


#endif