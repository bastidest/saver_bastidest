#include "scale_tranlate.h"

ScaleTranslate scale_proportional(int screen_width, int screen_height,
                                         int image_width, int image_height,
                                         scale_type_t scale_type) {
  double ratio_x = ((double)image_width) / ((double)screen_width);
  double ratio_y = ((double)image_height) / ((double)screen_height);

  ScaleTranslate ret;

  if ((scale_type == SCALE_TYPE_FIT && ratio_x > ratio_y) ||
      (scale_type == SCALE_TYPE_COVER && ratio_x < ratio_y)) {
    ret.scale_x = ret.scale_y = 1 / ratio_x;
    ret.translate_x = 0.;
    ret.translate_y =
        -(((double)(image_height)) / ratio_x - ((double)screen_height)) / (2.0);
  } else {
    ret.scale_y = ret.scale_x = 1 / ratio_y;
    ret.translate_y = 0.;
    ret.translate_x =
        -(((double)(image_width)) / ratio_y - ((double)screen_width)) / (2.0);
  }

  return ret;
}

ScaleTranslate scale_stretch(int screen_width, int screen_height,
                                    int image_width, int image_height) {
  ScaleTranslate ret;

  ret.scale_x = ((double)screen_width) / ((double)image_width);
  ret.scale_y = ((double)screen_height) / ((double)image_height);

  ret.translate_x = ret.translate_y = 0;

  return ret;
}

ScaleTranslate translate_center(int screen_width, int screen_height,
                                       int image_width, int image_height) {
  ScaleTranslate ret;

  int overflow_x = image_width - screen_width;
  int overflow_y = image_height - screen_height;

  ret.translate_x = -((double)overflow_x) / 2.;
  ret.translate_y = -((double)overflow_y) / 2.;

  ret.scale_x = ret.scale_y = 1.;

  return ret;
}
