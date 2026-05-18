#ifndef PRJ_CONFS_H
#define PRJ_CONFS_H

#include <stdint.h>

typedef enum {
  PRJ_CONFS_EXIT_FAIL = 0,
  PRJ_CONFS_EXIT_TIMEOUT,
  PRJ_CONFS_EXIT_CREDENTIALS,
} prj_confs_exit_t;

/** Bloqueia ate timeout PRJ_PAIR_TIMEOUT no primeiro boot (is_to_pair). BluFi ja deve estar ativo. */
prj_confs_exit_t prj_confs_boot_window(void);

/** Dados custom BluFi (JSON). Pode ser enviado a qualquer momento. */
void prj_confs_receive_data_cb(uint8_t *data, int len);

#endif
