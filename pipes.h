// SPDX-License-Identifier: GPL-2.0

#ifndef _inf_pipes_
#define _inf_pipes_

#include <pthread.h>
#include <stdbool.h>
#include "../rexusb/communication.h"

#define pipe_buffer_size 512
#define max_queue_buffers 10

static const char* dir_rexgen = "/var/run/rexgen";
static const char* can0 = "can0";
static const char* can1 = "can1";
static const char* gyro = "gyro";
static const char* acc = "acc";
static const char* dig = "dig";
static const char* adc = "adc";
static const char* sys = "sys";

static const uint io_interval = 1000; // 1 ms
static const uint nfc_io_cycles = 250;

typedef struct
{
	unsigned char data[max_queue_buffers][pipe_buffer_size];
	unsigned char qidx1;
	unsigned char qidx2;
} queuestruct;

// Main Usb object
DeviceStruct devobj;


// Main Communicaton thread
//static pthread_mutex_t mutex_io = PTHREAD_MUTEX_INITIALIZER;
pthread_t io;

// Main LiveData thread
static pthread_mutex_t mutex_usb = PTHREAD_MUTEX_INITIALIZER;
pthread_t usb;
queuestruct queue;


// Can0 TX pipe
static pthread_mutex_t mutex_can0_tx = PTHREAD_MUTEX_INITIALIZER;
pthread_t can0tx;
unsigned char data_can0tx[pipe_buffer_size];
unsigned long datasize_can0tx;

// Can0 RX pipe
static pthread_mutex_t mutex_can0_rx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  can0rx_cond = PTHREAD_COND_INITIALIZER;
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

// Gyro Rx pipe
static pthread_mutex_t mutex_gyro_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t gyrorx;
unsigned char data_gyrorx[pipe_buffer_size];
unsigned long datasize_gyrorx;

// Acc Rx pipe
static pthread_mutex_t mutex_acc_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t accrx;
unsigned char data_accrx[pipe_buffer_size];
unsigned long datasize_accrx;

// Digital Rx pipe
static pthread_mutex_t mutex_dig_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t digrx;
unsigned char data_digrx[pipe_buffer_size];
unsigned long datasize_digrx;

// ADC Rx pipe
static pthread_mutex_t mutex_adc_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t adcrx;
unsigned char data_adcrx[pipe_buffer_size];
unsigned long datasize_adcrx;


typedef char* TxCanRow[68];

bool InitPipes();
void DestroyPipes();

#endif
