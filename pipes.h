// SPDX-License-Identifier: GPL-2.0

#ifndef _inf_pipes_
#define _inf_pipes_

#include <pthread.h>
#include <stdbool.h>
#include "communication.h"

#define pipe_buffer_size 512
#define max_queue_buffers 10

static const unsigned short flag_can0 = 1 << 0;
static const unsigned short flag_can1 = 1 << 1;
static const unsigned short flag_can2 = 1 << 2;
static const unsigned short flag_can3 = 1 << 3;
static const unsigned short flag_gnss = 1 << 4;
static const unsigned short flag_gyro = 1 << 5;
static const unsigned short flag_acc = 1 << 6;
static const unsigned short flag_dig = 1 << 7;
static const unsigned short flag_adc = 1 << 8;

static const char* dir_rexgen = "/var/run/rexgen";
static const char* can0 = "can0";
static const char* can1 = "can1";
static const char* can2 = "can2";
static const char* can3 = "can3";
static const char* gnss = "gnss";
static const char* gyro = "gyro";
static const char* acc = "acc";
static const char* dig = "dig";
static const char* adc = "adc";
static const char* sys = "sys";
static const char* env = "env";

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

// Can2 TX pipe
static pthread_mutex_t mutex_can2_tx = PTHREAD_MUTEX_INITIALIZER;
pthread_t can2tx;
unsigned char data_can2tx[pipe_buffer_size];
unsigned long datasize_can2tx;

// Can2 RX pipe
static pthread_mutex_t mutex_can2_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t can2rx;
unsigned char data_can2rx[pipe_buffer_size];
unsigned long datasize_can2rx;

// Can3 TX pipe
static pthread_mutex_t mutex_can3_tx = PTHREAD_MUTEX_INITIALIZER;
pthread_t can3tx;
unsigned char data_can3tx[pipe_buffer_size];
unsigned long datasize_can3tx;

// Can3 RX pipe
static pthread_mutex_t mutex_can3_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t can3rx;
unsigned char data_can3rx[pipe_buffer_size];
unsigned long datasize_can3rx;

// Gnss Rx pipe
static pthread_mutex_t mutex_gnss_rx = PTHREAD_MUTEX_INITIALIZER;
pthread_t gnssrx;
unsigned char data_gnssrx[pipe_buffer_size];
unsigned long datasize_gnssrx;

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

bool InitPipes(unsigned short flags);
void DestroyPipes();

#endif
