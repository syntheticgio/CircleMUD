#!/bin/bash

protoc -I=../src/proto --python_out=./ ../src/proto/dawnmud.proto
