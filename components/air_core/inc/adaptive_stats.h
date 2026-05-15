#ifndef ADAPTIVE_STATS_H
#define ADAPTIVE_STATS_H

#include <stdint.h>

void adaptive_stats_reset(void);

void adaptive_stats_idle_push(int16_t pm25_spike, int16_t voc_spike);

void adaptive_stats_fill_event_thresholds(int16_t *thr_pm_start,
                                          int16_t *thr_voc_start,
                                          int16_t *thr_pm_active,
                                          int16_t *thr_voc_active,
                                          int16_t *thr_pm_end,
                                          int16_t *thr_voc_end,
                                          int16_t *thr_pm_idle,
                                          int16_t *thr_voc_idle,
                                          int16_t *adapt_vape_soft,
                                          int16_t *adapt_vape_med,
                                          int16_t *adapt_vape_high);

#endif
