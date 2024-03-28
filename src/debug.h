#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>



#define COLORRED    "\033[31m"
#define COLORGREEN  "\033[32m"

#define COLOREND    "\033[m" 

#define STARTBOLD   "\033[1m"
#define ENDBOLD     "\033[0m"

#define ERRMSG(MSG) STARTBOLD COLORRED MSG COLOREND 
#define SUCCESMSG(MSG) STARTBOLD COLORGREEN MSG COLOREND


#define assertVk(EX, MSG, SCSMSG) if(EX != VK_SUCCESS){ fprintf(stderr, "%s in %s->%s on line %d\n", ERRMSG(MSG), __FILE__, __func__, __LINE__); exit(EXIT_FAILURE);} else {fprintf(stdout, "%s\n", SUCCESMSG(SCSMSG));};
#define assert_my(EX, MSG, SCSMSG) if(!(EX)) { fprintf(stderr, " %s in %s->%s on line %d\n", ERRMSG(MSG), __FILE__, __func__, __LINE__); exit(EXIT_FAILURE);}else {fprintf(stdout, "%s\n", SUCCESMSG(SCSMSG));};


#define LOG(format, ...) fprintf (stderr, SUCCESMSG(format)"\n", ##__VA_ARGS__)

#endif