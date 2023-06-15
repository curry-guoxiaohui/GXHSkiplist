#!/bin/bash

make cleanp
make press
# 通过改变参数控制压力测试的执行次数
./press_test 1000000
echo  "press_test num : 500000"