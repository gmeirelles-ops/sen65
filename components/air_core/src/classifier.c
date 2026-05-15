#include "classifier.h"
#include "app_config.h"
#include <stdint.h>

static float safe_pm_voc_ratio(int16_t pm_spike, int16_t voc_spike) {
  if (voc_spike <= 1) {
    return 99.0f;
  }
  return (float)pm_spike / (float)voc_spike;
}

void classifier_process(const air_processed_data_t *data,
                        const air_event_t *event,
                        air_classification_t *result) {
  result->fire_score = 0;
  result->vape_score = 0;
  result->cig_score = 0;
  result->perfume_score = 0;
  result->dust_score = 0;
  result->alert_flags = 0;

  const int16_t pm = data->pm25_spike;
  const int16_t voc = data->voc_spike;
  const int16_t nox = data->nox_spike;
  const float r = safe_pm_voc_ratio(pm, voc);

  const int16_t vape_soft = event->adapt_vape_voc_soft;
  const int16_t vape_med = event->adapt_vape_voc_med;
  const int16_t vape_high = event->adapt_vape_voc_high;

  const bool cig_signature_nox =
      (((nox >= CIG_NOX_SPIKE_STRONG) && (r >= CIG_SIGNATURE_RATIO_MIN) &&
        (pm >= CIG_PM_SPIKE_SOFT) && (voc >= CIG_VOC_SPIKE_SOFT)) ||
       ((nox >= CIG_NOX_SPIKE_SOFT) && (r >= CIG_PM_VOC_RATIO_MIN) &&
        (pm >= CIG_PM_SPIKE_MED) && (voc >= 6)));

  int16_t co2_spike = 0;
  if (data->co2_valid && data->co2_ppm > 400) {
    uint16_t base = (data->co2_base > 400) ? data->co2_base : 450;
    if (data->co2_ppm > base) {
      co2_spike = (int16_t)(data->co2_ppm - base);
    }
  }

  const bool cig_signature_noxless =
      (nox < CIG_NOX_SPIKE_SOFT) && (pm >= CIG_NOXLESS_PM_SPIKE) &&
      (voc >= CIG_NOXLESS_VOC_SPIKE) && (r >= CIG_NOXLESS_RATIO_MIN) &&
      ((!data->co2_valid) || (co2_spike >= CIG_NOXLESS_CO2_SPIKE));

  const bool cig_signature = cig_signature_nox || cig_signature_noxless;

  // ----------------------------------------------------------------
  // 2. TRAVA DE SEGURANÇA: FAXINA PESADA OU BOM AR (Lockout)
  // ----------------------------------------------------------------
  // Gás altíssimo, sem partículas sólidas e odor persistente no ar
  if (voc > 120 && pm < 20 && event->duration > 45) {
    result->perfume_score = 90;
    if (event->flash_crowding) {
      result->alert_flags |= AIR_ALERT_FLASH_CROWDING;
    }
    return;
  }

  // Variáveis estáticas de detecção de variação térmica (Incêndio)
  static int16_t s_prev_temp_ticks;
  static bool s_have_prev_temp;

  // ================================================================
  // LÓGICA DE INCÊNDIO
  // ================================================================
  int16_t fire = 0;

  if (event->voc_led_suspect) {
    if (voc >= FIRE_PYROLYSIS_VOC_MIN && pm <= FIRE_PYROLYSIS_PM_MAX) {
      fire += FIRE_PYROLYSIS_VOC_BONUS;
      if (nox >= FIRE_PYROLYSIS_NOX_MIN) {
        fire += FIRE_PYROLYSIS_NOX_BONUS;
      }
      if (co2_spike >= FIRE_PYROLYSIS_CO2_SPIKE_MIN) {
        fire += FIRE_PYROLYSIS_CO2_BONUS;
      }
    }
    if (pm >= FIRE_PYROLYSIS_HANDOFF_PM_MIN &&
        voc >= FIRE_PYROLYSIS_HANDOFF_VOC_MIN) {
      fire += FIRE_PYROLYSIS_HANDOFF_BONUS;
    }
  }

  if (pm >= FIRE_PM_SPIKE_HIGH) {
    fire += 42;
  } else if (pm >= FIRE_PM_SPIKE_MED) {
    fire += 26;
  }

  if (pm >= FIRE_PM_VOC_COMBO_PM && voc >= FIRE_PM_VOC_COMBO_VOC) {
    fire += 28;
  }

  if (pm >= 110 && voc >= FIRE_VOC_SPIKE_HIGH) {
    fire += 24;
  } else if (pm >= 70 && voc >= FIRE_VOC_SPIKE_MED) {
    fire += 14;
  }

  if ((int32_t)event->duration >= FIRE_SUSTAIN_DURATION &&
      pm >= FIRE_SUSTAIN_PM_SPIKE && voc >= FIRE_SUSTAIN_VOC_SPIKE) {
    fire += 18;
  }

  if (event->peak_pm25 >= FIRE_PEAK_PM25_MIN && voc >= 6) {
    fire += 12;
  }

  if (s_have_prev_temp && pm >= 90) {
    int16_t d = (int16_t)(data->temp_ticks - s_prev_temp_ticks);
    if (d > FIRE_TEMP_DELTA_TICKS) {
      fire += 12;
    }
  }
  s_prev_temp_ticks = data->temp_ticks;
  s_have_prev_temp = true;

  if (pm >= 160 && voc <= FIRE_PM_SPIKE_DUST_VOC_MAX) {
    fire -= 35;
  }

  if (cig_signature) {
    fire -= CIG_DISAMBIG_FIRE_SUB;
  }

  if (fire < 0) fire = 0;
  if (fire > 100) fire = 100;
  result->fire_score = (uint8_t)fire;

  // ================================================================
  // LÓGICA DE VAPE (Otimizada para Banheiro Escolar)
  // ================================================================
  int16_t vape = 0;

  if (voc >= vape_high) {
    vape += 28;
  } else if (voc >= vape_med) {
    vape += 20;
  } else if (voc >= vape_soft) {
    vape += 12;
  }

  if (nox <= VAPE_NOX_SPIKE_MAX) {
    vape += 14;
  } else if (nox <= VAPE_NOX_SPIKE_MAX + 1) {
    vape += 6;
  }

  if (pm <= 25) {
    vape += 10;
  } else if (pm <= 55) {
    vape += 6;
  }

  if (pm > VAPE_PM_SPIKE_SOFT_CAP) {
    vape -= 18;
  }

  if (r >= VAPE_PM_VOC_RATIO_LOW && r <= VAPE_PM_VOC_RATIO_HIGH) {
    vape += 22;
  } else if (r < VAPE_PM_VOC_RATIO_LOW) {
    vape += 6;
  } else if (r <= 1.35f) {
    vape += 8;
  } else {
    vape -= 22;
  }

  if ((int32_t)event->duration >= 6 && voc >= vape_med) {
    vape += 10;
  }

  if (event->state == EVENT_DECAY) {
    if (event->decay_vape_shape >= 70) {
      vape += (int16_t)((DECAY_VAPE_SHAPE_BONUS * (int32_t)event->decay_vape_shape) /
                        100);
    }
    if (event->decay_dust_shape >= 60) {
      vape -= 25;
      result->dust_score = (uint8_t)((result->dust_score + 25 > 100)
                                         ? 100
                                         : (result->dust_score + 25));
    }
  }

  // --- NOVOS FILTROS DE VAPE PARA ESCOLA ---

  // Validação por CO2 (Exalação)
  if (co2_spike > 60) {
    vape += 25; // Forte evidência de bafo/pulmão soprando no sensor
  } else if (co2_spike < 15 && pm < 50) {
    vape -= 20; // Sem CO2 e com pouco PM? Provável borrifador ou perfume
  }

  // Proteção contra Odor Residual (Duração longa demais para um Vape)
  if ((int32_t)event->duration > 12 && pm < 100) {
    vape -= 30; 
  }

  // Proteção contra Poeira de Fundo gerando Razão 'r' perfeita por acidente
  if (data->pm25 < 150) { // Menos de 15 ug/m3 reais no ar
    if (vape > 20) vape -= 15;
  }

  // Penalidades por conflito com outras fontes
  if (pm >= FIRE_PM_SPIKE_MED && voc >= FIRE_VOC_SPIKE_MED) {
    vape -= 40;
  }
  if (nox >= CIG_NOX_SPIKE_STRONG && pm >= CIG_PM_SPIKE_SOFT) {
    vape -= 28;
  }
  if (r >= CIG_PM_VOC_RATIO_MIN && pm >= 160 && voc >= 7) {
    vape -= 20;
  }

  // ================================================================
  // LÓGICA DE CIGARRO TRADICIONAL
  // ================================================================
  int16_t cig = 0;

  if (pm >= CIG_PM_SPIKE_MED) {
    cig += 26;
  } else if (pm >= CIG_PM_SPIKE_SOFT) {
    cig += 16;
  }

  if (voc >= CIG_VOC_SPIKE_SOFT) {
    cig += 18;
  }

  if (nox >= CIG_NOX_SPIKE_STRONG) {
    cig += 30;
  } else if (nox >= CIG_NOX_SPIKE_SOFT) {
    cig += 14;
  }

  if (r >= CIG_PM_VOC_RATIO_MIN) {
    cig += 20;
  } else {
    cig -= 12;
  }

  if ((int32_t)event->duration >= 8 && pm >= 100 && voc >= 6) {
    cig += 12;
  }

  if (nox >= CIG_NOX_SPIKE_SOFT && pm >= 80 && voc >= 5) {
    cig += 10;
  }

  // Validação por CO2 (Exalação) para o Cigarro
  if (co2_spike > 60) {
    cig += 15;
  }

  if (cig_signature_noxless) {
    cig += CIG_NOXLESS_SCORE_BONUS;
  }

  if (r < 0.14f && voc >= 5) {
    cig -= 22;
  }

  if (pm > DUST_PM_THRESHOLD && voc < DUST_VOC_MAX) {
    cig -= 35;
  }

  if (pm >= FIRE_PM_SPIKE_HIGH && voc >= FIRE_VOC_SPIKE_HIGH) {
    if (!cig_signature) {
      cig -= 25;
    } else {
      cig -= 6;
    }
  }

  // ================================================================
  // PERFUME E POEIRA
  // ================================================================
  if (voc > PERFUME_VOC_THRESHOLD && pm < PERFUME_PM_MAX && nox < 2 &&
      (int32_t)event->duration < 15) {
    result->perfume_score = 80;
  }

  if (pm > DUST_PM_THRESHOLD && voc < DUST_VOC_MAX) {
    result->dust_score = 80;
  }

  // ================================================================
  // RESOLUÇÃO DE CONFLITOS E SUPRESSÃO MÚTUA
  // ================================================================
  if (result->fire_score >= 82) {
    int16_t cap = (int16_t)(result->fire_score - 75);
    if (cap < 0) {
      cap = 0;
    }
    if (cig_signature) {
      if (result->fire_score < CIG_FIRE_CAP_ONLY_ABOVE) {
        cap = 0;
      } else if (cap > 10) {
        cap = 10;
      }
    }
    vape -= cap;
    cig -= cap;
  }

  if (vape < 0) vape = 0;
  if (vape > 100) vape = 100;
  result->vape_score = (uint8_t)vape;

  if (cig < 0) cig = 0;
  if (cig > 100) cig = 100;
  result->cig_score = (uint8_t)cig;

  // Se for muito mais provável ser Vape, derruba o score de Cigarro
  if (result->vape_score > result->cig_score + 12 &&
      result->fire_score < FIRE_SCORE_THRESHOLD) {
    result->cig_score =
        (uint8_t)((result->cig_score > 15) ? (result->cig_score - 12) : 0);
  }

  // Se for muito mais provável ser Cigarro, derruba o score de Vape
  if (result->cig_score > result->vape_score + 12 &&
      result->fire_score < FIRE_SCORE_THRESHOLD) {
    result->vape_score =
        (uint8_t)((result->vape_score > 15) ? (result->vape_score - 12) : 0);
  }

  if (event->flash_crowding) {
    result->alert_flags |= AIR_ALERT_FLASH_CROWDING;
  }
}