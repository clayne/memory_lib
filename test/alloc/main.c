/* main.c  -  Memory allocation tests  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform memory allocation library in C11 providing basic support data types and
 * functions to write applications and games in a platform-independent fashion. The latest source code is
 * always available at
 *
 * https://github.com/rampantpixels/memory_lib
 *
 * This library is built on top of the foundation library available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>
#include <test/test.h>

#include <memory/memory.h>
#include <memory/log.h>

#include <stdio.h>

static application_t
test_alloc_application(void) {
	application_t app;
	memset(&app, 0, sizeof(app));
	app.name = string_const(STRING_CONST("Memory alloc tests"));
	app.short_name = string_const(STRING_CONST("test_alloc"));
	app.company = string_const(STRING_CONST("Rampant Pixels"));
	app.flags = APPLICATION_UTILITY;
	app.exception_handler = test_exception_handler;
	return app;
}

static foundation_config_t
test_alloc_config(void) {
	foundation_config_t config;
	memset(&config, 0, sizeof(config));
	return config;
}

static memory_system_t
test_alloc_memory_system(void) {
	return memory_system_malloc();
}

static int 
test_alloc_initialize(void) {
	log_set_suppress(HASH_MEMORY, ERRORLEVEL_DEBUG);
	return 0;
}

static void 
test_alloc_finalize(void) {
}

DECLARE_TEST(alloc, alloc) {
	unsigned int iloop = 0;
	unsigned int ipass = 0;
	unsigned int icheck = 0;
	unsigned int id = 0;
	void* addr[8142];
	char data[20000];
	unsigned int datasize[7] = { 473, 39, 195, 24, 73, 376, 245 };

	memory_system_t memsys = memory_system();

	memsys.initialize();
	memsys.thread_finalize();
	memsys.finalize();

	memsys.initialize();

	for (id = 0; id < 20000; ++id)
		data[id] = (char)(id % 139 + id % 17);

	for (iloop = 0; iloop < 64; ++iloop) {
		for (ipass = 0; ipass < 8142; ++ipass) {
			addr[ipass] = memsys.allocate(0, 500, 0, MEMORY_PERSISTENT);
			EXPECT_NE(addr[ipass], 0);

			memcpy(addr[ipass], data, 500);

			for (icheck = 0; icheck < ipass; ++icheck) {
				EXPECT_NE(addr[icheck], addr[ipass]);
				if (addr[icheck] < addr[ipass])
					EXPECT_LT(pointer_offset(addr[icheck], 500),
					          addr[ipass]);     //LT since we have some bookkeeping overhead in memory manager between blocks
				else if (addr[icheck] > addr[ipass])
					EXPECT_LT(pointer_offset(addr[ipass], 500), addr[icheck]);
			}
		}

		for (ipass = 0; ipass < 8142; ++ipass)
			EXPECT_EQ(memcmp(addr[ipass], data, 500), 0);

		for (ipass = 0; ipass < 8142; ++ipass)
			memsys.deallocate(addr[ipass]);
	}

	for (iloop = 0; iloop < 64; ++iloop) {
		for (ipass = 0; ipass < 1024; ++ipass) {
			unsigned int cursize = datasize[ipass%7] + ipass;

			addr[ipass] = memsys.allocate(0, cursize, 0, MEMORY_PERSISTENT);
			EXPECT_NE(addr[ipass], 0);

			memcpy(addr[ipass], data, cursize);

			for (icheck = 0; icheck < ipass; ++icheck) {
				EXPECT_NE(addr[icheck], addr[ipass]);
				/*if( addr[icheck] < addr[ipass] )
					EXPECT_LT( pointer_offset( addr[icheck], cursize ), addr[ipass] ); //LT since we have some bookkeeping overhead in memory manager between blocks
				else if( addr[icheck] > addr[ipass] )
				EXPECT_LT( pointer_offset( addr[ipass], cursize ), addr[icheck] );*/
			}
		}

		for (ipass = 0; ipass < 1024; ++ipass) {
			unsigned int cursize = datasize[ipass%7] + ipass;
			EXPECT_EQ(memcmp(addr[ipass], data, cursize), 0);
		}

		for (ipass = 0; ipass < 1024; ++ipass)
			memsys.deallocate(addr[ipass]);
	}

	for (iloop = 0; iloop < 128; ++iloop) {
		for (ipass = 0; ipass < 1024; ++ipass) {
			addr[ipass] = memsys.allocate(0, 500, 0, MEMORY_PERSISTENT);
			EXPECT_NE(addr[ipass], 0);

			memcpy(addr[ipass], data, 500);

			for (icheck = 0; icheck < ipass; ++icheck) {
				EXPECT_NE(addr[icheck], addr[ipass]);
				if (addr[icheck] < addr[ipass])
					EXPECT_LT(pointer_offset(addr[icheck], 500),
					          addr[ipass]);     //LT since we have some bookkeeping overhead in memory manager between blocks
				else if (addr[icheck] > addr[ipass])
					EXPECT_LT(pointer_offset(addr[ipass], 500), addr[icheck]);
			}
		}

		for (ipass = 0; ipass < 1024; ++ipass) {
			EXPECT_EQ(memcmp(addr[ipass], data, 500), 0);
		}

		for (ipass = 0; ipass < 1024; ++ipass)
			memsys.deallocate(addr[ipass]);
	}

	memsys.thread_finalize();
	memsys.finalize();

	return 0;
}

typedef struct _allocator_thread_arg {
	memory_system_t     memory_system;
	unsigned int        loops;
	unsigned int        passes; //max 4096
	unsigned int        datasize[32];
	unsigned int        num_datasize; //max 32
} allocator_thread_arg_t;

static void* 
allocator_thread(void* argp) {
	allocator_thread_arg_t arg = *(allocator_thread_arg_t*)argp;
	memory_system_t memsys = arg.memory_system;
	unsigned int iloop = 0;
	unsigned int ipass = 0;
	unsigned int icheck = 0;
	unsigned int id = 0;
	void* addr[4096];
	char data[8192];
	unsigned int cursize;
	unsigned int iwait = 0;

	for (id = 0; id < 8192; ++id)
		data[id] = (char)id;

	iwait = random32_range(0, 10);
	thread_sleep(iwait);

	for (iloop = 0; iloop < arg.loops; ++iloop) {
		for (ipass = 0; ipass < arg.passes; ++ipass) {
			cursize = arg.datasize[(iloop + ipass + iwait) % arg.num_datasize ] + (iloop % 1024);

			addr[ipass] = memsys.allocate(0, cursize, 0, MEMORY_PERSISTENT);
			EXPECT_NE(addr[ipass], 0);

			memcpy(addr[ipass], data, cursize);

			for (icheck = 0; icheck < ipass; ++icheck) {
				EXPECT_NE(addr[icheck], addr[ipass]);
				if (addr[icheck] < addr[ipass])
					EXPECT_LE(pointer_offset(addr[icheck], cursize), addr[ipass]);
				else if (addr[icheck] > addr[ipass])
					EXPECT_LE(pointer_offset(addr[ipass], cursize), addr[icheck]);
			}
		}

		for (ipass = 0; ipass < arg.passes; ++ipass) {
			cursize = arg.datasize[(iloop + ipass + iwait) % arg.num_datasize ] + (iloop % 1024);

			EXPECT_EQ(memcmp(addr[ipass], data, cursize), 0);
			memsys.deallocate(addr[ipass]);
		}
	}

	memsys.thread_finalize();

	return 0;
}

DECLARE_TEST(alloc, threaded) {
	thread_t thread[32];
	void* threadres[32];
	unsigned int i;
	size_t num_alloc_threads;
#if BUILD_ENABLE_DETAILED_MEMORY_STATISTICS
	volatile memory_statistics_detail_t stat;
#endif
	allocator_thread_arg_t thread_arg;
	memory_system_t memsys = memory_system();
	memsys.initialize();

	num_alloc_threads = system_hardware_threads() + 1;
	if (num_alloc_threads < 3)
		num_alloc_threads = 3;
	if (num_alloc_threads > 32)
		num_alloc_threads = 32;

#if BUILD_ENABLE_DETAILED_MEMORY_STATISTICS
	stat = memory_statistics_detailed();
	log_memory_info(STRING_CONST("STATISTICS AFTER INITIALIZE"));
	log_memory_infof(STRING_CONST("Virtual current size: %" PRIu64), stat.allocated_current_virtual);
	log_memory_infof(STRING_CONST("Current size:         %" PRIu64), stat.allocated_current);
	log_memory_info(STRING_CONST(""));
	log_memory_infof(STRING_CONST("Virtual total size:   %" PRIu64), stat.allocated_total_virtual);
	log_memory_infof(STRING_CONST("Total size:           %" PRIu64), stat.allocated_total);
	log_memory_info(STRING_CONST(""));
	log_memory_infof(STRING_CONST("Virtual count:        %" PRIu64), stat.allocations_current_virtual);
	log_memory_infof(STRING_CONST("Count:                %" PRIu64), stat.allocations_current);
	log_memory_info(STRING_CONST(""));
	log_memory_infof(STRING_CONST("Virtual total count:  %" PRIu64), stat.allocations_total_virtual);
	log_memory_infof(STRING_CONST("Total count:          %" PRIu64), stat.allocations_total);
#endif

	//Warm-up
	thread_arg.memory_system = memsys;
	thread_arg.loops = 100000;
	thread_arg.passes = 1024;
	thread_arg.datasize[0] = 19;
	thread_arg.datasize[1] = 249;
	thread_arg.datasize[2] = 797;
	thread_arg.datasize[3] = 3;
	thread_arg.datasize[4] = 79;
	thread_arg.datasize[5] = 34;
	thread_arg.datasize[6] = 389;
	thread_arg.num_datasize = 7;

	EXPECT_EQ(allocator_thread(&thread_arg), 0);

	for (i = 0; i < 7; ++i)
		thread_arg.datasize[i] = 500;
	EXPECT_EQ(allocator_thread(&thread_arg), 0);

	thread_arg.datasize[0] = 19;
	thread_arg.datasize[1] = 249;
	thread_arg.datasize[2] = 797;
	thread_arg.datasize[3] = 3;
	thread_arg.datasize[4] = 79;
	thread_arg.datasize[5] = 34;
	thread_arg.datasize[6] = 389;
	thread_arg.num_datasize = 7;

	for (i = 0; i < num_alloc_threads; ++i) {
		thread_initialize(thread + i, allocator_thread, &thread_arg, STRING_CONST("allocator"), THREAD_PRIORITY_NORMAL, 0);
		thread_start(thread + i);
	}

	test_wait_for_threads_startup(thread, num_alloc_threads);
	test_wait_for_threads_finish(thread, num_alloc_threads);

	for (i = 0; i < num_alloc_threads; ++i) {
		threadres[i] = thread_join(thread + i);
		thread_finalize(thread + i);
	}

#if BUILD_ENABLE_DETAILED_MEMORY_STATISTICS
	stat = memory_statistics_detailed();
	log_memory_info(STRING_CONST("STATISTICS AFTER TEST"));
	log_memory_infof(STRING_CONST("Virtual current size: %" PRIu64), stat.allocated_current_virtual);
	log_memory_infof(STRING_CONST("Current size:         %" PRIu64), stat.allocated_current);
	log_memory_info(STRING_CONST(""));
	log_memory_infof(STRING_CONST("Virtual total size:   %" PRIu64), stat.allocated_total_virtual);
	log_memory_infof(STRING_CONST("Total size:           %" PRIu64), stat.allocated_total);
	log_memory_info(STRING_CONST(""));
	log_memory_infof(STRING_CONST("Virtual count:        %" PRIu64), stat.allocations_current_virtual);
	log_memory_infof(STRING_CONST("Count:                %" PRIu64), stat.allocations_current);
	log_memory_info(STRING_CONST(""));
	log_memory_infof(STRING_CONST("Virtual total count:  %" PRIu64), stat.allocations_total_virtual);
	log_memory_infof(STRING_CONST("Total count:          %" PRIu64), stat.allocations_total);
#endif

	memsys.thread_finalize();
	memsys.finalize();

#if BUILD_ENABLE_DETAILED_MEMORY_STATISTICS
	stat = memory_statistics_detailed();
	log_memory_info(STRING_CONST("STATISTICS AFTER SHUTDOWN"));
	log_memory_infof(STRING_CONST("Virtual current size: %" PRIu64), stat.allocated_current_virtual);
	log_memory_infof(STRING_CONST("Current size:         %" PRIu64), stat.allocated_current);
	log_memory_info(STRING_CONST(""));
	log_memory_infof(STRING_CONST("Virtual total size:   %" PRIu64), stat.allocated_total_virtual);
	log_memory_infof(STRING_CONST("Total size:           %" PRIu64), stat.allocated_total);
	log_memory_info(STRING_CONST(""));
	log_memory_infof(STRING_CONST("Virtual count:        %" PRIu64), stat.allocations_current_virtual);
	log_memory_infof(STRING_CONST("Count:                %" PRIu64), stat.allocations_current);
	log_memory_info(STRING_CONST(""));
	log_memory_infof(STRING_CONST("Virtual total count:  %" PRIu64), stat.allocations_total_virtual);
	log_memory_infof(STRING_CONST("Total count:          %" PRIu64), stat.allocations_total);
#endif

	for (i = 0; i < num_alloc_threads; ++i)
		EXPECT_EQ(threadres[i], 0);

	return 0;
}

static void 
test_alloc_declare(void) {
	ADD_TEST(alloc, alloc);
	ADD_TEST(alloc, threaded);
}

static test_suite_t test_alloc_suite = {
	test_alloc_application,
	test_alloc_memory_system,
	test_alloc_config,
	test_alloc_declare,
	test_alloc_initialize,
	test_alloc_finalize,
	0
};

#if FOUNDATION_PLATFORM_ANDROID

int
test_alloc_run(void);

int
test_alloc_run(void) {
	test_suite = test_alloc_suite;
	return test_run_all();
}

#else

test_suite_t 
test_suite_define(void);

test_suite_t 
test_suite_define(void) {
	return test_alloc_suite;
}

#endif
