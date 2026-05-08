#ifndef APP_CONFIG_H
#define APP_CONFIG_H

//====================================================
// SENSOR TASK & QUEUE
//====================================================
#define SENSOR_QUEUE_SIZE           10
#define SENSOR_READ_INTERVAL_MS     1000
#define SENSOR_WARMUP_TIME_MS       5000

#define SENSOR_TASK_STACK           4096
#define SENSOR_TASK_PRIORITY        5

#define PROCESS_TASK_STACK          4096
#define PROCESS_TASK_PRIORITY       4

//====================================================
// FILTERS (EMA)
//====================================================
#define EMA_FAST_ALPHA              20
#define EMA_FAST_DIV                100

//====================================================
// BASELINE & NOISE
//====================================================
#define BASELINE_ALPHA              5
#define BASELINE_DIV                500

#define PM_NOISE_FLOOR              2
#define VOC_NOISE_FLOOR             5
#define NOX_NOISE_FLOOR             1

//====================================================
// EVENT THRESHOLDS (Hysteresis)
//====================================================
#define EVENT_PM_START_THRESHOLD    40
#define EVENT_VOC_START_THRESHOLD   100

#define EVENT_PM_ACTIVE_THRESHOLD   80
#define EVENT_VOC_ACTIVE_THRESHOLD  80

#define EVENT_PM_END_THRESHOLD      15
#define EVENT_VOC_END_THRESHOLD     20

#define EVENT_PM_IDLE_THRESHOLD     5
#define EVENT_VOC_IDLE_THRESHOLD    10

//====================================================
// DETECTION THRESHOLDS
//====================================================
#define DETECTION_SCORE_THRESHOLD   70

// VAPE
#define VAPE_VOC_THRESHOLD          80
#define VAPE_NOX_MAX                5

// CIGARRO
#define CIG_PM_THRESHOLD            150
#define CIG_VOC_THRESHOLD           80
#define CIG_NOX_THRESHOLD           10

// PERFUME
#define PERFUME_VOC_THRESHOLD       120
#define PERFUME_PM_MAX              20

// POEIRA
#define DUST_PM_THRESHOLD           150
#define DUST_VOC_MAX                20

#endif