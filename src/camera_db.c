#include "camera_db.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define EARTH_RADIUS_KM 6371.0

static const CameraLocation CAMERA_DB[] = {
    {"Helsinki / Kehä I", 60.2206, 24.8710},
    {"Turku / Satakunnantie", 60.4517, 22.2666},
    {"Tampere / Hervanta", 61.4499, 23.8560},
    {"Oulu / Limingantulli", 65.0057, 25.4711},
};

static double degrees_to_radians(double degrees)
{
    return degrees * (M_PI / 180.0);
}

size_t camera_db_count(void)
{
    return sizeof(CAMERA_DB) / sizeof(CAMERA_DB[0]);
}

const CameraLocation *camera_db_entries(void)
{
    return CAMERA_DB;
}

double camera_db_distance_km(double lat_a, double lon_a, double lat_b, double lon_b)
{
    const double delta_lat = degrees_to_radians(lat_b - lat_a);
    const double delta_lon = degrees_to_radians(lon_b - lon_a);
    const double a = sin(delta_lat / 2.0) * sin(delta_lat / 2.0) +
                     cos(degrees_to_radians(lat_a)) * cos(degrees_to_radians(lat_b)) *
                         sin(delta_lon / 2.0) * sin(delta_lon / 2.0);
    const double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    return EARTH_RADIUS_KM * c;
}

const CameraLocation *camera_db_find_nearest(double latitude, double longitude)
{
    const size_t entry_count = camera_db_count();

    if (entry_count == 0U)
    {
        return NULL;
    }

    const CameraLocation *nearest = &CAMERA_DB[0];
    double nearest_distance = camera_db_distance_km(
        latitude,
        longitude,
        nearest->latitude,
        nearest->longitude);

    for (size_t index = 1; index < entry_count; ++index)
    {
        const CameraLocation *candidate = &CAMERA_DB[index];
        const double candidate_distance = camera_db_distance_km(
            latitude,
            longitude,
            candidate->latitude,
            candidate->longitude);

        if (candidate_distance < nearest_distance)
        {
            nearest = candidate;
            nearest_distance = candidate_distance;
        }
    }

    return nearest;
}
