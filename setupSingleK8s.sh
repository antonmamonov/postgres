#!/bin/bash

kubectl create -f ./k8s/storage.yaml
kubectl create -f ./k8s/statefulset.yaml
