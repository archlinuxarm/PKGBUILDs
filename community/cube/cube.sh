#!/bin/sh

cd /opt/cube
artsshell suspend &> /dev/null
esdctl standby &> /dev/null
./cube_client "$@"
esdctl resume &> /dev/null