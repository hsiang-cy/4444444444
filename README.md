### 1. 健康檢查

```
GET /health
```

**回應範例**：
```json
{
  "status": "ok"
}
```

### 2. 取得可用策略

```
GET /strategies
```

**回應範例**：
```json
{
  "strategies": [
    "SAVINGS",
    "PATH_CHEAPEST_ARC",
    "GLOBAL_CHEAPEST_ARC",
    "PATH_MOST_CONSTRAINED_ARC",
    "CHRISTOFIDES",
    "PARALLEL_CHEAPEST_INSERTION",
    "LOCAL_CHEAPEST_INSERTION",
    "ALL_UNPERFORMED"
  ]
}
```

### 3. 求解VRP問題

```
POST /vrp
```

## 請求參數詳解

### 必要參數

| 參數名稱 | 類型 | 說明 |
|---------|------|------|
| `distance_matrix` | 二維數組 | 各地點間的距離矩陣。`distance_matrix[i][j]`表示從地點i到地點j的距離 |
| `demands` | 整數數組 | 各地點的需求量。索引0通常是倉庫(depot)，需求量為0 |
| `vehicle_capacities` | 整數數組 | 每台車的載重容量 |

### 選擇性參數

| 參數名稱 | 類型 | 預設值 | 說明 |
|---------|------|--------|------|
| `depot` | 整數 | 0 | 倉庫位置的索引 |
| `num_vehicles` | 整數 | 容量數組長度 | 車輛數量。若不指定，系統會根據需求自動決定最少車輛數 |
| `fixed_costs` | 數字數組 | 自動設定 | 每台車的固定成本。高值會促使系統減少車輛使用 |
| `strategy` | 字串 | "PATH_CHEAPEST_ARC" | 求解策略（見策略說明） |
| `time_limit` | 整數 | 10 | 求解時間限制（秒） |

## 求解策略說明

| 策略名稱 | 特點 | 適用場景 |
|---------|------|----------|
| `SAVINGS` | 經典節約算法，快速 | 需要快速得到不錯解的場景 |
| `PATH_CHEAPEST_ARC` | 逐步構建路徑，平衡 | 一般用途，預設選擇 |
| `GLOBAL_CHEAPEST_ARC` | 全局視角選擇 | 追求較優解的場景 |
| `CHRISTOFIDES` | TSP專用，保證品質 | 單車輛或類TSP問題 |
| `PARALLEL_CHEAPEST_INSERTION` | 並行插入 | 大規模問題 |

## 回應格式

### 成功回應
```json
{
  "success": true,
  "routes": [[路線1], [路線2], ...],
  "vehicles_used": 實際使用車輛數,
  "total_distance": 總行駛距離,
  "strategy_used": "使用的策略名稱"
}
```

### 失敗回應
```json
{
  "success": false,
  "error": "錯誤訊息",
  "strategy_used": "使用的策略名稱"
}
```

## 使用範例

### 情景1：固定車輛數配送

**背景**：配送中心有3台車，每台載重15單位，需要配送到8個客戶點。

**請求**：
```bash
curl -X POST http://localhost:8000/vrp \
  -H "Content-Type: application/json" \
  -d '{
    "distance_matrix": [
      [0, 548, 776, 696, 582, 274, 502, 194, 308],
      [548, 0, 684, 308, 194, 502, 730, 354, 696],
      [776, 684, 0, 992, 878, 502, 274, 810, 468],
      [696, 308, 992, 0, 114, 650, 878, 502, 844],
      [582, 194, 878, 114, 0, 536, 764, 388, 730],
      [274, 502, 502, 650, 536, 0, 228, 308, 194],
      [502, 730, 274, 878, 764, 228, 0, 536, 194],
      [194, 354, 810, 502, 388, 308, 536, 0, 342],
      [308, 696, 468, 844, 730, 194, 194, 342, 0]
    ],
    "depot": 0,
    "demands": [0, 1, 1, 2, 4, 2, 4, 8, 8],
    "vehicle_capacities": [15, 15, 15],
    "num_vehicles": 3,
    "time_limit": 30
  }'
```

**回應**：
```json
{
  "success": true,
  "routes": [
    [0, 7, 1, 4, 3, 0],
    [0, 5, 2, 6, 8, 0]
  ],
  "vehicles_used": 2,
  "total_distance": 3104,
  "strategy_used": "PATH_CHEAPEST_ARC"
}
```

**解讀**：
- 系統優化後只需要2台車（比提供的3台少）
- 車輛1路線：倉庫→客戶7→客戶1→客戶4→客戶3→倉庫（載重15單位）
- 車輛2路線：倉庫→客戶5→客戶2→客戶6→客戶8→倉庫（載重15單位）

### 情景2：最小化車輛數

**背景**：電商倉庫需要配送90單位貨物到11個地點，每台車容量40單位，希望用最少車輛完成。

**請求**：
```bash
curl -X POST http://localhost:8000/vrp \
  -H "Content-Type: application/json" \
  -d '{
    "distance_matrix": [
      [0, 548, 776, 696, 582, 274, 502, 194, 308, 636, 502, 388],
      [548, 0, 684, 308, 194, 502, 730, 354, 696, 742, 1084, 594],
      [776, 684, 0, 992, 878, 502, 274, 810, 468, 742, 400, 1278],
      [696, 308, 992, 0, 114, 650, 878, 502, 844, 890, 1232, 514],
      [582, 194, 878, 114, 0, 536, 764, 388, 730, 776, 1118, 400],
      [274, 502, 502, 650, 536, 0, 228, 308, 194, 536, 502, 388],
      [502, 730, 274, 878, 764, 228, 0, 536, 194, 468, 354, 1016],
      [194, 354, 810, 502, 388, 308, 536, 0, 342, 388, 730, 468],
      [308, 696, 468, 844, 730, 194, 194, 342, 0, 274, 388, 810],
      [636, 742, 742, 890, 776, 536, 468, 388, 274, 0, 342, 536],
      [502, 1084, 400, 1232, 1118, 502, 354, 730, 388, 342, 0, 878],
      [388, 594, 1278, 514, 400, 388, 1016, 468, 810, 536, 878, 0]
    ],
    "depot": 0,
    "demands": [0, 5, 10, 7, 8, 12, 15, 6, 9, 11, 4, 3],
    "vehicle_capacities": [40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40],
    "fixed_costs": [5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000],
    "time_limit": 30
  }'
```

**回應**：
```json
{
  "success": true,
  "routes": [
    [0, 8, 5, 0],
    [0, 9, 10, 2, 6, 0],
    [0, 11, 4, 3, 1, 7, 0]
  ],
  "vehicles_used": 3,
  "total_distance": 4024,
  "strategy_used": "PATH_CHEAPEST_ARC"
}
```

**解讀**：
- 系統計算出最少需要3台車（總需求90 ÷ 容量40 = 2.25，實際需要3台）
- 高固定成本成功引導系統最小化車輛使用
- 車輛利用率達75%

### 情景3：大規模配送優化

**背景**：物流公司需要配送101單位貨物到15個客戶，有4台容量35的貨車可用。

**請求**：
```bash
curl -X POST http://localhost:8000/vrp \
  -H "Content-Type: application/json" \
  -d '{
    "distance_matrix": [16x16矩陣...],
    "depot": 0,
    "demands": [0, 3, 5, 9, 4, 8, 6, 10, 7, 11, 2, 12, 5, 8, 4, 7],
    "vehicle_capacities": [35, 35, 35, 35],
    "num_vehicles": 4,
    "strategy": "CHRISTOFIDES",
    "time_limit": 120
  }'
```

**回應**：
```json
{
  "success": true,
  "routes": [
    [0, 15, 9, 12, 11, 0],
    [0, 7, 3, 4, 1, 5, 0],
    [0, 8, 6, 2, 14, 13, 10, 0]
  ],
  "vehicles_used": 3,
  "total_distance": 5410,
  "strategy_used": "CHRISTOFIDES"
}
```

**解讀**：
- 雖然有4台車可用，系統優化後只需3台
- 車輛利用率高達96.2%
- CHRISTOFIDES策略在2分鐘內找到優質解

## 並行策略使用建議

為獲得最佳結果，建議同時嘗試多個策略：

```javascript
// Node.js 範例
const strategies = ['SAVINGS', 'PATH_CHEAPEST_ARC', 'GLOBAL_CHEAPEST_ARC'];
const results = await Promise.all(
    strategies.map(strategy => 
        axios.post('http://localhost:8000/vrp', {...data, strategy})
    )
);
// 選擇車輛數最少或總距離最短的結果
```

## 注意事項

1. **容量限制**：確保總需求不超過所有車輛的總容量
2. **距離矩陣**：必須是方形矩陣，且對角線為0
3. **時間限制**：複雜問題可能需要更長時間（建議30-120秒）
4. **車輛數量**：若不指定`num_vehicles`，請確保`vehicle_capacities`數組足夠長

## 錯誤處理

常見錯誤：
- `"vehicles_ == vehicle_capacities.size()"`: 車輛數與容量數組長度不匹配
- `"No solution found"`: 問題無解（如需求超過總容量）
- 超時：增加`time_limit`參數值