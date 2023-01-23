# Containerized postgreSQL with Kubernetes

## Quick Kubernetes setup

### Quick deploy with persistent data volume

1. First, create a secret to store the password for the postgres user. Replace `notagoodpassword` with your password
```bash
kubectl create secret generic single-postgres-password \
    --from-literal=postgrespassword=notagoodpassword

kubectl get secrets single-postgres-password
```

2. Now deploy the volume and stateful set
```bash
./setupSingleK8s.sh
```

### Quick removal 

```bash
kubectl delete -f ./k8s/statefulset.yaml

# To delete the volume as well WARNING: THIS WILL DELETE ALL YOUR DATA!
kubectl delete -f ./k8s/storage.yaml
```

## Quick local docker setup

### Quick build

```bash
./buildDocker.sh
```

### Quick run locally

```bash
# create a very basic password for 'postgres' user
export PGPASSWORD=notagoodpassword

# WARNING DON'T THIS FOR PRODUCTION!
```

```bash
$ ./runDockerProdLocal.sh
```

**NOTE: if needed to manually start the process**
```bash
$ ./runDockerDevLocal.sh
# inside running container
$ /home/postgres/entrypoint.sh
```