#!/bin/bash
set -e

IMAGE_NAME="drone_fleet_runtime:latest"
CONTAINER_NAME="drone_fleet_instance"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE}")" && pwd)"

echo "=== Building High-Efficiency Multi-Stage Docker Layer Optimization Matrix ==="
docker build -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile" "$SCRIPT_DIR"

echo "=== Booting Containerized Drone Fleet Workspace Cluster Stack ==="
docker run -it --rm \
  --name "$CONTAINER_NAME" \
  --net=host \
  -e ROS_DOMAIN_ID=12 \
  -v "$SCRIPT_DIR/src/drone_fleet:/ros2_ws/src/drone_fleet:rw" \
  "$IMAGE_NAME"
