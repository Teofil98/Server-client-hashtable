#pragma once

#include <cstdint>
#define SEM_NAME "/semaphore"
#define SHM_NAME "shared_mem"
#define SHM_SIZE 4096

using uint32 = uint32_t;
using int32 = int32_t;

enum class OPERATION : int32 {
	INSERT,
	DELETE,
	FIND,
	PRINT,
	QUIT
};

union bytes_to_int32_t {
	char bytes[4];
	int32 integer;
};
