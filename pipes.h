// SPDX-License-Identifier: GPL-2.0

#ifndef _inf_pipes_
#define _inf_pipes_

#include <pthread.h>
#include <stdbool.h>

#define pipe_buffer_size 512

static const char* dir_rexgen = "/var/run/rexgen";
static const char* can0 = "can0";
static const char* can1 = "can1";
static const char* gyro = "gyro";
static const char* acc = "acc";

// Can0 TX pipe
static pthread_mutex_t mutex_can0_tx = PTHREAD_MUTEX_INITIALIZER;
pthread_t can0tx;
unsigned char data_can0tx[pipe_buffer_size];
unsigned long datasize_can0tx;

// Can0 RX pipe
static pthread_mutex_t mutex_can0_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t can0rx;
unsigned char data_can0rx[pipe_buffer_size];
unsigned long datasize_can0rx;

// Can1 TX pipe
static pthread_mutex_t mutex_can1_tx = PTHREAD_MUTEX_INITIALIZER;
pthread_t can1tx;
unsigned char data_can1tx[pipe_buffer_size];
unsigned long datasize_can1tx;

// Can1 RX pipe
static pthread_mutex_t mutex_can1_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t can1rx;
unsigned char data_can1rx[pipe_buffer_size];
unsigned long datasize_can1rx;

// Gyro Tx pipe
//static pthread_mutex_t mutex_gyro_tx = PTHREAD_MUTEX_INITIALIZER;
//pthread_t gyrotx;
//unsigned char data_gyrotx[pipe_buffer_size];
//unsigned long datasize_gyrotx;

// Gyro Rx pipe
static pthread_mutex_t mutex_gyro_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t gyrorx;
unsigned char data_gyrorx[pipe_buffer_size];
unsigned long datasize_gyrorx;

// Acc Tx pipe
//static pthread_mutex_t mutex_acc_tx = PTHREAD_MUTEX_INITIALIZER;
//pthread_t acctx;
//unsigned char data_acctx[pipe_buffer_size];
//unsigned long datasize_acctx;

// Gyro Rx pipe
static pthread_mutex_t mutex_acc_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t accrx;
unsigned char data_accrx[pipe_buffer_size];
unsigned long datasize_accrx;

typedef char* TxCanRow[68];

bool InitPipes();
void DestroyPipes();

#endif
