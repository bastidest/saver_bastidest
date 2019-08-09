typedef enum _scale_type {
  SCALE_TYPE_STRETCH,
  SCALE_TYPE_FIT,
  SCALE_TYPE_COVER,
  SCALE_TYPE_CENTER
} scale_type_t;

typedef struct {
  double translate_x;
  double translate_y;
  double scale_x;
  double scale_y;
} ScaleTranslate;

ScaleTranslate scale_proportional(int screen_width, int screen_height,
                                         int image_width, int image_height,
                                         scale_type_t scale_type);

ScaleTranslate scale_stretch(int screen_width, int screen_height,
                                    int image_width, int image_height);

ScaleTranslate translate_center(int screen_width, int screen_height,
                                       int image_width, int image_height);
