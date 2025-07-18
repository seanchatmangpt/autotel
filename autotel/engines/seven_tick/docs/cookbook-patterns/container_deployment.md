# Pattern: Containerization and Orchestration

## Description
This pattern demonstrates how to containerize the 7T Engine using Docker and deploy it in a scalable, orchestrated environment using Docker Compose and Kubernetes.

## Dockerfile Example
```dockerfile
# Multi-stage build for 7T Engine
FROM ubuntu:20.04 AS builder
RUN apt-get update && apt-get install -y build-essential
COPY . /src
WORKDIR /src
RUN make clean && make

FROM ubuntu:20.04
COPY --from=builder /src/verification/sparql_simple_test /app/
COPY --from=builder /src/lib/lib7t_runtime.so /usr/local/lib/
RUN ldconfig
WORKDIR /app
CMD ["./sparql_simple_test"]
```

## Docker Compose Example
```yaml
version: '3.8'
services:
  seven-tick:
    build: .
    ports:
      - "8080:8080"
    volumes:
      - seven_tick_data:/var/lib/seven-tick/data
    environment:
      - MAX_SUBJECTS=1000000
      - MAX_PREDICATES=10000
      - MAX_OBJECTS=1000000
    restart: unless-stopped
volumes:
  seven_tick_data:
```

## Kubernetes Deployment Example
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: seven-tick
spec:
  replicas: 3
  selector:
    matchLabels:
      app: seven-tick
  template:
    metadata:
      labels:
        app: seven-tick
    spec:
      containers:
      - name: seven-tick
        image: seven-tick:latest
        ports:
        - containerPort: 8080
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: seven-tick-service
spec:
  selector:
    app: seven-tick
  ports:
  - port: 80
    targetPort: 8080
  type: LoadBalancer
```

## Tips
- Use multi-stage builds to minimize image size.
- Set resource requests and limits for production deployments.
- Use health and readiness probes for robust orchestration.
- Persist data using Docker volumes or Kubernetes Persistent Volumes. 