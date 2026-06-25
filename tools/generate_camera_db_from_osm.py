#!/usr/bin/env python3
"""Generate src/camera_db.c from OpenStreetMap Finland speed camera data."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import urllib.parse
import urllib.request

OVERPASS_URL = "https://overpass-api.de/api/interpreter"
OVERPASS_QUERY = """
[out:json][timeout:180];
area["ISO3166-1"="FI"][admin_level=2]->.searchArea;
(
  node["highway"="speed_camera"](area.searchArea);
);
out body;
"""


def c_escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def camera_name(tags: dict[str, str], osm_id: int) -> str:
    candidates = (
        tags.get("name"),
        tags.get("description"),
        tags.get("ref"),
    )
    for candidate in candidates:
        if candidate:
            normalized = re.sub(r"\s+", " ", candidate).strip()
            if normalized:
                return normalized
    return f"OSM speed camera {osm_id}"


def fetch_overpass() -> dict:
    payload = urllib.parse.urlencode({"data": OVERPASS_QUERY}).encode("utf-8")
    request = urllib.request.Request(
        OVERPASS_URL,
        data=payload,
        headers={"User-Agent": "IoT-Laite OSM camera database generator"},
    )
    with urllib.request.urlopen(request, timeout=240) as response:
        return json.load(response)


def parse_cameras(payload: dict) -> list[tuple[str, float, float]]:
    cameras: list[tuple[str, float, float]] = []
    for element in payload.get("elements", []):
        if element.get("type") != "node":
            continue
        latitude = element.get("lat")
        longitude = element.get("lon")
        if latitude is None or longitude is None:
            continue
        tags = element.get("tags", {})
        name = camera_name(tags, element.get("id", 0))
        cameras.append((name, float(latitude), float(longitude)))
    cameras.sort(key=lambda item: (item[1], item[2], item[0]))
    return cameras


def render_camera_db(cameras: list[tuple[str, float, float]]) -> str:
    lines = [
        '#include "camera_db.h"',
        "",
        "#include <math.h>",
        "",
        "#ifndef M_PI",
        "#define M_PI 3.14159265358979323846",
        "#endif",
        "",
        "#define EARTH_RADIUS_KM 6371.0",
        "",
        "/*",
        " * Generated from OpenStreetMap data (highway=speed_camera, Finland).",
        " * Source query: Overpass API.",
        " */",
        "static const CameraLocation CAMERA_DB[] = {",
    ]

    for name, latitude, longitude in cameras:
        lines.append(f'    {{"{c_escape(name)}", {latitude:.7f}, {longitude:.7f}}},')

    lines.extend(
        [
            "};",
            "",
            "static double degrees_to_radians(double degrees)",
            "{",
            "    return degrees * (M_PI / 180.0);",
            "}",
            "",
            "size_t camera_db_count(void)",
            "{",
            "    return sizeof(CAMERA_DB) / sizeof(CAMERA_DB[0]);",
            "}",
            "",
            "const CameraLocation *camera_db_entries(void)",
            "{",
            "    return CAMERA_DB;",
            "}",
            "",
            "double camera_db_distance_km(double lat_a, double lon_a, double lat_b, double lon_b)",
            "{",
            "    const double delta_lat = degrees_to_radians(lat_b - lat_a);",
            "    const double delta_lon = degrees_to_radians(lon_b - lon_a);",
            "    const double a = sin(delta_lat / 2.0) * sin(delta_lat / 2.0) +",
            "                     cos(degrees_to_radians(lat_a)) * cos(degrees_to_radians(lat_b)) *",
            "                         sin(delta_lon / 2.0) * sin(delta_lon / 2.0);",
            "    const double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));",
            "",
            "    return EARTH_RADIUS_KM * c;",
            "}",
            "",
            "const CameraLocation *camera_db_find_nearest(double latitude, double longitude)",
            "{",
            "    const size_t entry_count = camera_db_count();",
            "",
            "    if (entry_count == 0U)",
            "    {",
            "        return NULL;",
            "    }",
            "",
            "    const CameraLocation *nearest = &CAMERA_DB[0];",
            "    double nearest_distance = camera_db_distance_km(",
            "        latitude,",
            "        longitude,",
            "        nearest->latitude,",
            "        nearest->longitude);",
            "",
            "    for (size_t index = 1; index < entry_count; ++index)",
            "    {",
            "        const CameraLocation *candidate = &CAMERA_DB[index];",
            "        const double candidate_distance = camera_db_distance_km(",
            "            latitude,",
            "            longitude,",
            "            candidate->latitude,",
            "            candidate->longitude);",
            "",
            "        if (candidate_distance < nearest_distance)",
            "        {",
            "            nearest = candidate;",
            "            nearest_distance = candidate_distance;",
            "        }",
            "    }",
            "",
            "    return nearest;",
            "}",
            "",
        ]
    )

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate camera_db.c from Finland OSM speed cameras."
    )
    parser.add_argument(
        "--input",
        type=pathlib.Path,
        help="Optional local Overpass JSON file. If omitted, data is downloaded.",
    )
    parser.add_argument(
        "--output",
        type=pathlib.Path,
        required=True,
        help="Path to generated C source (for example: src/camera_db.c).",
    )
    args = parser.parse_args()

    if args.input:
        payload = json.loads(args.input.read_text(encoding="utf-8"))
    else:
        payload = fetch_overpass()

    cameras = parse_cameras(payload)
    if not cameras:
        raise SystemExit("No speed cameras found in the input data.")

    generated = render_camera_db(cameras)
    args.output.write_text(generated, encoding="utf-8")
    print(f"Wrote {len(cameras)} cameras to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
