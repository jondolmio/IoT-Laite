#ifndef CAMERA_DB_H
#define CAMERA_DB_H

#include <stddef.h>

typedef struct
{
    const char *name;
    double latitude;
    double longitude;
} CameraLocation;

size_t camera_db_count(void);
const CameraLocation *camera_db_entries(void);
const CameraLocation *camera_db_find_nearest(double latitude, double longitude);
double camera_db_distance_km(double lat_a, double lon_a, double lat_b, double lon_b);

#endif
