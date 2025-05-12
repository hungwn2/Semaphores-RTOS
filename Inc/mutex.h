/*
 * mutex.h
 *
 *  Created on: May 4, 2025
 *      Author: User
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include <stdbool.h>
#include <stddef.h>
#include "queue.h"
#include "rtos_status.h"
#include "task.h"

typedef struct{
	bool locked;
	uint8_t priority;
	TaskControlBlock* owner;
	Queue waiting_tasks;
};

RTOSSTATUS init_mutex(Mutex* mutex);
RTOSStatus lock_mutex(Mutex* mutex);
RTOSStatus unlock_mutex(Mutex* mutex);

#endif /* MUTEX_H_ */
