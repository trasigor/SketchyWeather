#include <pebble.h>
#include "bitmap_weather_ids.h"

int get_bitmap_id(int weather_id, bool small) {
  static int image_id = 0;
  
  if (weather_id == 1010) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_CLEAN_SKY_DAY;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_CLEAN_SKY_DAY;
    }
  }
  else if (weather_id == 1020) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_CLEAN_SKY_NIGHT;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_CLEAN_SKY_NIGHT;
    }
  }
  else if (weather_id == 1110) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_FEW_CLOUDS_DAY;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_FEW_CLOUDS_DAY;
    }
  }
  else if (weather_id == 1120) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_FEW_CLOUDS_NIGHT;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_FEW_CLOUDS_NIGHT;
    }
  }
  else if (weather_id >= 200 && weather_id <= 232) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_RAIN_THUNDERSTORM;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_RAIN_THUNDERSTORM;
    }
  }
  else if ((weather_id >= 300 && weather_id <= 504) || (weather_id >= 520 && weather_id <= 531)) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_RAIN;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_RAIN;
    }
  }
  else if (weather_id == 511 || (weather_id >= 611 && weather_id <= 616)) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_RAIN_SNOW;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_RAIN_SNOW;
    }
  }
  else if ((weather_id >= 600 && weather_id <= 602) || (weather_id >= 620 && weather_id <= 622)) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_SNOW;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_SNOW;
    }
  }
  else if (weather_id >= 701 && weather_id <= 762) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_MIST_NIGHT;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_MIST_NIGHT;
    }
  }
  else if (weather_id == 771 || weather_id == 905 || (weather_id >= 952 && weather_id <= 957)) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_WIND;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_WIND;
    }
  }
  else if (weather_id == 781 || (weather_id >= 900 && weather_id <= 902) || (weather_id >= 958 && weather_id <= 962)) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_TORNADO;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_TORNADO;
    }
  }
  else if ((weather_id >= 802 && weather_id <= 804) || weather_id == 903 || weather_id == 951) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_CLOUDS;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_CLOUDS;
    }
  }
  else if (weather_id == 906) {
    if (small) {
      image_id = RESOURCE_ID_IMAGE_SMALL_THUNDERSTORM;
    }
    else {
      image_id = RESOURCE_ID_IMAGE_THUNDERSTORM;
    }
  }
  
  return image_id;
}