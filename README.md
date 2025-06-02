# Description
AIRouting2.0 algorithm algorithms-server.



# Initialization
```shell=
bash init.sh this
```



# Development
Initialize the development environment and enter.
```shell=
docker compose up -d air2.0-dev
docker exec -it air2.0-dev bash
```

Compile and execute in development environment.
```shell=
bash build.sh all
```



# Build Image
Development
```shell=
version=$(grep '  VERSION' CMakeLists.txt | awk '{print $2}')
docker buildx build --platform linux/arm64,linux/amd64 \
-t docker-hub.saico.systems/airouting2/algorithms-server-development:${version} \
-t docker-hub.saico.systems/airouting2/algorithms-server-development:latest \
. --push
```

Production
```shell=
version=$(grep '  VERSION' CMakeLists.txt | awk '{print $2}')
docker buildx build --platform linux/arm64,linux/amd64 \
-t docker-hub.saico.systems/airouting2/algorithms-server-production:${version} \
-t docker-hub.saico.systems/airouting2/algorithms-server-production:latest \
. --push
```



# Execute Image
Please adjust the following settings of air2.0-min in docker-compose.yml first:
| Name     | Description           | Range        |
| -------- | --------              | --------     |
| image    | Docker image          | Unsigned Int |
| ports    | Algorithm Server Port | Unsigned Int |

Please adjust the following settings in .env first:
| Name                 | Description                     | Range                     |
| --------             | --------                        | --------                  |
| WEB_SERVER_PORT      | Algorithm Server Port           | Unsigned Int              |
| MAX_MEMORY_USAGE_PCT | Maximum memory usage percentage | 0.2 to 0.8<br>-1: Disable |
| LOG_DIR              | Log directory                   | null: Disable             |
| REDIS_IP             | Redis ip                        |                           |
| REDIS_PORT           | Redis port                      | Unsigned Int              |
| REDIS_TIMEOUT        | Redis timeout                   | >= 0.1                    |
| REDIS_AUTH           | Redis auth                      |                           |
| REDIS_INDEX_SET      | Redis index set                 | Unsigned Int              |
| REDIS_INDEX_PUB      | Redis index pub                 | Unsigned Int              |
| REDIS_DATA_TTL       | Redis data ttl                  | Unsigned Int              |
| AWS_REGION           | AWS region                      | String                    |
| AWS_ACCESS_KEY_ID    | AWS access key id               | String                    |
| AWS_SECRET_ACCESS_KEY| AWS secret access key           | String                    |
| AWS_S3_BUCKET_NAME   | AWS S3 bucket name              | String                    |

and then execute:
```shell=
docker compose up -d air2.0-min
```



# Account
| Module Code | Module Name       |
| --------    | --------          |
| 110         | ALGORITHMS_SERVER |



# Operator
| Operator Code | Operator Name       | Description | Http Method | url                 |
| --------      | --------            | --------    | --------    | --------            |
| 1001          | PING                | PING        | POST        | ping                |
| 1002          | GET_CONFIG          | 取得設定　　  | POST        | get_config          |
| 1003          | AIR_ALGORITHM       | 演算法核心　  | POST        | air_algorithm       |



# Test
PING
```shell=
curl -i -X POST "http://127.0.0.1:4210/ping"
```

GET_CONFIG
```shell=
curl -i -X POST "http://127.0.0.1:4210/get_config"
```

AIR_ALGORITHM
```shell=
curl -i -X POST "http://127.0.0.1:4210/air_algorithm" \
-H "accept: application/json" \
-H "Content-Type: application/json" \
-d '{
    "taskId": "1111",
    "s3Dir":  "test"
}'
```
