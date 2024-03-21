@echo off
if exist "build" (rd /s/q "build") else (echo good!)

md build
cd build
cmake ..
make -j

md results