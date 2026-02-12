# single_check_signal_status 函数流程图

## 函数功能
检查所有信号灯状态，包括所有灯全灭、单方向信号灯不亮、单个相位灯不亮。

## 流程图

```mermaid
flowchart TD
    
    
    Start([开始]) --> LoopType[遍历灯类型<br/>远灯, 近灯]
    LoopType --> LoopDir[遍历方向<br/>北 东 南 西]
    
    LoopDir --> LoopRoad[遍历道路类型<br/>主道 辅道]
    LoopRoad --> LoopPhase[遍历相位<br/>左转 直行 右转 行人1 行人2 非机动车1 非机动车2 掉头 可变 逆向可变 潮汐]
    LoopPhase --> LoopColor[遍历颜色<br/>红 黄 绿]
    
    LoopColor --> CheckPtr{电压和电流<br/>是否配置通道?}
    CheckPtr -->|否| NextColor[继续下一个颜色]
    CheckPtr --> ReadValue[读取电压和电流值]
    
    ReadValue --> CheckLight{当前是否有电压 且<br/>电流>10mA?}
    CheckLight -->|是| SetLightOn[灯亮<br/>light_dir_off = 0<br/>light_all_off = 0]
    CheckLight -->|否| NextColor
    
    SetLightOn --> NextColor
    NextColor --> NextPhase{颜色遍历完成?}
    NextPhase -->|否| LoopColor
    NextPhase -->|是| BreakPhase{方向不全灭?}
    
    BreakPhase -->|是| BreakRoad[跳出相位循环]
    BreakPhase -->|否| NextRoad{相位遍历完成?}
    NextRoad -->|否| LoopPhase
    NextRoad -->|是| BreakDir{方向不全灭?}
    
    BreakDir -->|是| BreakType[跳出道路循环]
    BreakDir -->|否| NextDir{道路遍历完成?}
    NextDir -->|否| LoopRoad
    NextDir -->|是| CheckDirOff{方向全灭且<br/>有有效通道?}
    
    CheckDirOff -->|是| SetFault[设置故障<br/>TrafficFault_Set<br/>TRAFFIC_PART_NO_LIGHT]
    SetFault --> SetErrorCode["设置错误码<br/>error_code.fault |= 1 &lt;&lt; LED_PART_NO_LIGHT<br/>error_code.type |= 1 &lt;&lt; type<br/>error_code.dir |= 1 &lt;&lt; dir"]
    SetErrorCode --> NextDirCheck
    CheckDirOff -->|否| ClearFault[清除故障<br/>TrafficFault_Clear<br/>TRAFFIC_PART_NO_LIGHT]
    ClearFault --> NextDirCheck
    
    NextDirCheck{方向遍历完成?}
    NextDirCheck -->|否| LoopDir
    NextDirCheck -->|是| NextType{灯类型遍历完成?}
    NextType -->|否| LoopType
    NextType -->|是| CheckAllOff{所有灯全灭?}
    
    CheckAllOff -->|是| SetAllOffFault["设置全灭故障<br/>error_code.fault |= 1 &lt;&lt; LED_ALL_NO_LIGHT<br/>TrafficFault_Set<br/>TRAFFIC_ALL_NO_LIGHT"]
    SetAllOffFault --> ClearAllDirFault[清除所有方向不亮故障<br/>遍历type和dir<br/>TrafficFault_Clear<br/>TRAFFIC_PART_NO_LIGHT]
    ClearAllDirFault --> Return
    CheckAllOff -->|否| ClearAllOffFault[清除全灭故障<br/>TrafficFault_Clear<br/>TRAFFIC_ALL_NO_LIGHT]
    ClearAllOffFault --> Return
    
    Return([返回 error_code])
    
    style Start fill:#90EE90
    style Return fill:#FFB6C1
    style CheckPtr fill:#87CEEB
    style CheckLight fill:#87CEEB
    style BreakPhase fill:#87CEEB
    style BreakDir fill:#87CEEB
    style CheckDirOff fill:#87CEEB
    style CheckAllOff fill:#87CEEB
    style SetFault fill:#FF6347
    style SetAllOffFault fill:#FF6347
    style ClearFault fill:#32CD32
    style ClearAllOffFault fill:#32CD32
    style ClearAllDirFault fill:#32CD32
```

## 变量说明

| 变量名 | 类型 | 说明 |
|--------|------|------|
| type | Type_e | 灯类型：远灯/近灯 |
| dir | Direction_e | 方向：北/东/南/西 |
| road | RoadType_e | 道路类型：主道/辅道 |
| phase | Phase_e | 相位：左转/直行/右转/人行1/人行2/非机动车1/非机动车2/掉头/可变/逆向/潮汐 |
| color | Color_e | 颜色：红/绿/黄 |
| current_value | float | 电流值 |
| voltage_value | uint8_t | 电压值 |
| light_dir_off | uint8_t | 方向是否全灭：1=全灭，0=不全灭 |
| light_all_off | uint8_t | 所有灯是否全灭：1=全灭，0=不全灭 |
| has_valid_channel | uint8_t | 是否有有效通道：1=有，0=无 |
| error_code | ErrorCode_t | 错误码结构 |

## 故障类型

| 故障类型 | 说明 |
|----------|------|
| TRAFFIC_PART_NO_LIGHT | 单方向信号灯不亮 |
| TRAFFIC_ALL_NO_LIGHT | 所有灯全灭 |
| LED_PART_NO_LIGHT | 单方向不亮故障位 |
| LED_ALL_NO_LIGHT | 全灭故障位 |

## 逻辑说明

1. **灯亮判断条件**：电压为低电平（0）且电流大于10mA
2. **方向全灭判断**：方向内所有灯都不亮（light_dir_off = 1）且有有效通道
3. **全灭判断**：所有灯都不亮（light_all_off = 1）
4. **故障处理**：
   - 当方向全灭时，设置单方向不亮故障
   - 当所有灯全灭时，设置全灭故障并清除所有方向不亮故障
   - 当有灯亮时，清除对应方向的故障