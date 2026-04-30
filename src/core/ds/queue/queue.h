#ifndef QUEUE_H
#define QUEUE_H

#include "error/error.h"
#include "ds/tree/node.h"

typedef TreeNode* QueueUnit;

typedef struct Queue {
	QueueUnit data;
	struct Queue* next;
} Queue;

Error enqueue(Queue** queue, QueueUnit data);
Error dequeue(Queue** queue, QueueUnit* data);

#endif
