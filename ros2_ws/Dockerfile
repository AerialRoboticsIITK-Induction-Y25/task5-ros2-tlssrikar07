FROM ros:humble AS builder
SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    python3-colcon-common-extensions \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ros2_ws/src/drone_fleet
COPY . .

WORKDIR /ros2_ws
RUN source /opt/ros/humble/setup.bash && \
    colcon build --merge-install --cmake-args -DCMAKE_BUILD_TYPE=Release

FROM ros:humble-ros-core AS runtime
SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ros2_ws
COPY --from=builder /ros2_ws/install ./install

RUN echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc && \
    echo "source /ros2_ws/install/setup.bash" >> ~/.bashrc

RUN echo '#!/bin/bash' > /entrypoint.sh && \
    echo 'source /opt/ros/humble/setup.bash' >> /entrypoint.sh && \
    echo 'source /ros2_ws/install/setup.bash' >> /entrypoint.sh && \
    echo 'exec "$@"' >> /entrypoint.sh && \
    chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["ros2", "launch", "drone_fleet", "fleet.launch.py"]
