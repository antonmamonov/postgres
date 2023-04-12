#!/bin/bash

kubectl port-forward -n default svc/single-pg-postgresql 5432:5432