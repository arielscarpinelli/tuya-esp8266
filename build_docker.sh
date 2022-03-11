#!/bin/sh
docker run -it --mount type=bind,source="$(pwd)",target=/opt/build -w /opt/build python:2 bash $*
