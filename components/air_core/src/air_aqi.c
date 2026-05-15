#include "air_aqi.h"

#include <math.h>
#include <stdint.h>

typedef struct {
  int i_low;
  int i_high;
  float c_low;
  float c_high;
} aqi_breakpoint_t;

/* Tabela EPA (Technical Assistance Document for AQI), PM2.5 24 h. */
static const aqi_breakpoint_t s_pm25_bp[] = {
    {0, 50, 0.0f, 12.0f},       {51, 100, 12.1f, 35.4f},
    {101, 150, 35.5f, 55.4f},   {151, 200, 55.5f, 150.4f},
    {201, 300, 150.5f, 250.4f}, {301, 400, 250.5f, 350.4f},
    {401, 500, 350.5f, 500.4f},
};

static const aqi_breakpoint_t s_pm10_bp[] = {
    {0, 50, 0.0f, 54.0f},       {51, 100, 55.0f, 154.0f},
    {101, 150, 155.0f, 254.0f}, {151, 200, 255.0f, 354.0f},
    {201, 300, 355.0f, 424.0f}, {301, 400, 425.0f, 504.0f},
    {401, 500, 505.0f, 604.0f},
};

static int aqi_from_concentration(float c,
                                  const aqi_breakpoint_t *bp,
                                  unsigned n_bp) {
  if (c < 0.f) {
    return -1;
  }
  if (c <= bp[0].c_low) {
    return 0;
  }
  for (unsigned i = 0; i < n_bp; i++) {
    if (c <= bp[i].c_high) {
      float denom = bp[i].c_high - bp[i].c_low;
      if (denom <= 0.f) {
        return bp[i].i_low;
      }
      float i_f =
          ((float)(bp[i].i_high - bp[i].i_low) / denom) * (c - bp[i].c_low) +
          (float)bp[i].i_low;
      int i_round = (int)lroundf(i_f);
      if (i_round < 0) {
        i_round = 0;
      }
      if (i_round > 500) {
        i_round = 500;
      }
      return i_round;
    }
  }
  return 500;
}

int air_aqi_pm25_from_ugm3(float pm25_ugm3) {
  return aqi_from_concentration(pm25_ugm3, s_pm25_bp,
                                (unsigned)(sizeof s_pm25_bp / sizeof s_pm25_bp[0]));
}

int air_aqi_pm10_from_ugm3(float pm10_ugm3) {
  return aqi_from_concentration(pm10_ugm3, s_pm10_bp,
                                (unsigned)(sizeof s_pm10_bp / sizeof s_pm10_bp[0]));
}

void air_aqi_fill_info(int aqi, air_aqi_info_t *out) {
  if (out == NULL) {
    return;
  }
  out->aqi = aqi;
  if (aqi < 0) {
    out->category_pt = "Indisponivel";
    out->category_en = "Unavailable";
    out->color_name = "gray";
    out->color_hex = "#9CA3AF";
    return;
  }
  if (aqi <= 50) {
    out->category_pt = "Bom";
    out->category_en = "Good";
    out->color_name = "green";
    out->color_hex = "#00E400";
  } else if (aqi <= 100) {
    out->category_pt = "Moderado";
    out->category_en = "Moderate";
    out->color_name = "yellow";
    out->color_hex = "#FFFF00";
  } else if (aqi <= 150) {
    out->category_pt = "Insalubre p/ sensiveis";
    out->category_en = "Unhealthy for Sensitive Groups";
    out->color_name = "orange";
    out->color_hex = "#FF7E00";
  } else if (aqi <= 200) {
    out->category_pt = "Insalubre";
    out->category_en = "Unhealthy";
    out->color_name = "red";
    out->color_hex = "#FF0000";
  } else if (aqi <= 300) {
    out->category_pt = "Muito insalubre";
    out->category_en = "Very Unhealthy";
    out->color_name = "purple";
    out->color_hex = "#8F3F97";
  } else {
    out->category_pt = "Perigoso";
    out->category_en = "Hazardous";
    out->color_name = "maroon";
    out->color_hex = "#7E0023";
  }
}

int air_aqi_category_code(int aqi) {
  if (aqi < 0) {
    return -1;
  }
  if (aqi <= 50) {
    return 0;
  }
  if (aqi <= 100) {
    return 1;
  }
  if (aqi <= 150) {
    return 2;
  }
  if (aqi <= 200) {
    return 3;
  }
  if (aqi <= 300) {
    return 4;
  }
  return 5;
}

bool air_aqi_overall_from_readings(float pm25_ugm3, float pm10_ugm3,
                                   air_aqi_info_t *out) {
  int a25 = air_aqi_pm25_from_ugm3(pm25_ugm3);
  int a10 = air_aqi_pm10_from_ugm3(pm10_ugm3);
  int overall = -1;
  if (a25 >= 0 && a10 >= 0) {
    overall = (a25 > a10) ? a25 : a10;
  } else if (a25 >= 0) {
    overall = a25;
  } else if (a10 >= 0) {
    overall = a10;
  } else {
    return false;
  }
  air_aqi_fill_info(overall, out);
  return true;
}
