#!/usr/bin/env bash

for file in examples/*
do
  printf "Running example: %s...\n" "$file"
  ./ced "$file"
  echo
done
