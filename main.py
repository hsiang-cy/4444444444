from fastapi import FastAPI
from ortools.constraint_solver import routing_enums_pb2
from ortools.constraint_solver import pywrapcp

app = FastAPI()

# 策略映射表
STRATEGIES = {
    "SAVINGS": routing_enums_pb2.FirstSolutionStrategy.SAVINGS,
    "PATH_CHEAPEST_ARC": routing_enums_pb2.FirstSolutionStrategy.PATH_CHEAPEST_ARC,
    "GLOBAL_CHEAPEST_ARC": routing_enums_pb2.FirstSolutionStrategy.GLOBAL_CHEAPEST_ARC,
    "PATH_MOST_CONSTRAINED_ARC": routing_enums_pb2.FirstSolutionStrategy.PATH_MOST_CONSTRAINED_ARC,
    "CHRISTOFIDES": routing_enums_pb2.FirstSolutionStrategy.CHRISTOFIDES,
    "PARALLEL_CHEAPEST_INSERTION": routing_enums_pb2.FirstSolutionStrategy.PARALLEL_CHEAPEST_INSERTION,
    "LOCAL_CHEAPEST_INSERTION": routing_enums_pb2.FirstSolutionStrategy.LOCAL_CHEAPEST_INSERTION,
    "ALL_UNPERFORMED": routing_enums_pb2.FirstSolutionStrategy.ALL_UNPERFORMED
}

@app.post("/vrp")
async def solve_vrp(data: dict):
    try:
        # 核心參數
        distance_matrix = data["distance_matrix"]
        num_locations = len(distance_matrix)
        depot = data.get("depot", 0)
        demands = data["demands"]
        vehicle_capacities = data["vehicle_capacities"]
        
        # 策略選擇
        strategy_name = data.get("strategy", "PATH_CHEAPEST_ARC")
        strategy = STRATEGIES.get(strategy_name, routing_enums_pb2.FirstSolutionStrategy.PATH_CHEAPEST_ARC)
        
        # 車輛數處理 - 修正邏輯
        if "num_vehicles" in data:
            num_vehicles = data["num_vehicles"]
        else:
            # 如果沒指定車輛數，使用提供的容量陣列長度
            num_vehicles = len(vehicle_capacities)
        
        # 確保容量陣列長度正確
        if len(vehicle_capacities) < num_vehicles:
            # 如果容量陣列太短，用最後一個值填充
            last_capacity = vehicle_capacities[-1] if vehicle_capacities else 100
            vehicle_capacities = vehicle_capacities + [last_capacity] * (num_vehicles - len(vehicle_capacities))
        else:
            # 如果容量陣列太長，截斷
            vehicle_capacities = vehicle_capacities[:num_vehicles]
        
        # 固定成本處理
        if "fixed_costs" in data:
            fixed_costs = data["fixed_costs"]
            if len(fixed_costs) < num_vehicles:
                last_cost = fixed_costs[-1] if fixed_costs else 10000
                fixed_costs = fixed_costs + [last_cost] * (num_vehicles - len(fixed_costs))
            else:
                fixed_costs = fixed_costs[:num_vehicles]
        else:
            # 根據是否要最小化車輛數來設定
            fixed_costs = [10000 if "num_vehicles" not in data else 0] * num_vehicles
        
        # 建立模型（後續代碼相同）
        manager = pywrapcp.RoutingIndexManager(num_locations, num_vehicles, depot)
        routing = pywrapcp.RoutingModel(manager)
        
        # 距離回調
        def distance_callback(from_index, to_index):
            from_node = manager.IndexToNode(from_index)
            to_node = manager.IndexToNode(to_index)
            return int(distance_matrix[from_node][to_node])
        
        transit_callback_index = routing.RegisterTransitCallback(distance_callback)
        routing.SetArcCostEvaluatorOfAllVehicles(transit_callback_index)
        
        # 固定成本
        for vehicle_id in range(num_vehicles):
            routing.SetFixedCostOfVehicle(fixed_costs[vehicle_id], vehicle_id)
        
        # 容量約束
        def demand_callback(from_index):
            from_node = manager.IndexToNode(from_index)
            return demands[from_node]
        
        demand_callback_index = routing.RegisterUnaryTransitCallback(demand_callback)
        routing.AddDimensionWithVehicleCapacity(
            demand_callback_index, 0, vehicle_capacities, True, "Capacity"
        )
        
        # 求解參數
        search_parameters = pywrapcp.DefaultRoutingSearchParameters()
        search_parameters.first_solution_strategy = strategy
        search_parameters.local_search_metaheuristic = (
            routing_enums_pb2.LocalSearchMetaheuristic.GUIDED_LOCAL_SEARCH
        )
        search_parameters.time_limit.seconds = data.get("time_limit", 10)
        
        # 求解
        solution = routing.SolveWithParameters(search_parameters)
        
        if solution:
            routes = []
            total_distance = 0
            
            for vehicle_id in range(num_vehicles):
                if not routing.IsVehicleUsed(solution, vehicle_id):
                    continue
                    
                route = []
                route_distance = 0
                index = routing.Start(vehicle_id)
                
                while not routing.IsEnd(index):
                    node = manager.IndexToNode(index)
                    route.append(node)
                    prev_index = index
                    index = solution.Value(routing.NextVar(index))
                    route_distance += distance_callback(prev_index, index)
                
                route.append(depot)
                
                if len(route) > 2:
                    routes.append(route)
                    total_distance += route_distance
            
            return {
                "success": True,
                "routes": routes,
                "vehicles_used": len(routes),
                "total_distance": total_distance,
                "strategy_used": strategy_name
            }
        
        return {"success": False, "error": "No solution found", "strategy_used": strategy_name}
        
    except Exception as e:
        return {"success": False, "error": str(e)}

@app.get("/strategies")
async def get_strategies():
    """回傳可用的策略列表"""
    return {"strategies": list(STRATEGIES.keys())}

@app.get("/health")
async def health():
    return {"status": "ok"}