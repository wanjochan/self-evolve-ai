# UI 控制 API 文档

本文档提供了 UI 控制服务器的 API 文档。该服务器基于 FastAPI 构建，提供与桌面 GUI 交互的端点。

当服务器运行时，API 文档也可以通过 Swagger UI (`/docs`) 和 ReDoc (`/redoc`) 访问。

## 基础 URL

`http://127.0.0.1:8000`

## 认证

当前版本不需要认证。

---

## API 端点

### 1. 列出窗口

*   **端点**: `GET /windows`
*   **描述**: 获取当前所有打开的顶层窗口列表。
*   **成功响应**:
    *   **代码**: `200 OK`
    *   **内容**: `application/json`
    ```json
    [
        {
            "id": "132746",
            "title": "计算器"
        },
        {
            "id": "65538",
            "title": "无标题 - 记事本"
        }
    ]
    ```

### 2. 获取窗口信息

*   **端点**: `GET /windows/{window_id}`
*   **描述**: 获取指定窗口的详细信息。
*   **路径参数**:
    *   `window_id` (string, 必需): 窗口的唯一标识符。
*   **成功响应**:
    *   **代码**: `200 OK`
    *   **内容**: `application/json`
    ```json
    {
        "id": "132746",
        "title": "计算器",
        "position": { "x": 100, "y": 100 },
        "size": { "width": 320, "height": 500 },
        "process_id": 12345
    }
    ```
*   **错误响应**:
    *   **代码**: `404 Not Found`
    *   **内容**: `application/json`
    ```json
    {
        "detail": "Window not found"
    }
    ```

### 3. 获取窗口截图

*   **端点**: `GET /windows/{window_id}/screenshot`
*   **描述**: 捕获指定窗口的截图。
*   **路径参数**:
    *   `window_id` (string, 必需): 窗口的唯一标识符。
*   **成功响应**:
    *   **代码**: `200 OK`
    *   **内容**: `image/png`
    *   响应体为原始的 PNG 图像数据。
*   **错误响应**:
    *   **代码**: `404 Not Found`
    *   **内容**: `application/json`
    ```json
    {
        "detail": "Window not found"
    }
    ```

### 4. 控制窗口

*   **端点**: `POST /windows/{window_id}/control`
*   **描述**: 对指定窗口执行操作（例如：移动、调整大小、关闭）。
*   **路径参数**:
    *   `window_id` (string, 必需): 窗口的唯一标识符。
*   **请求体**: `application/json`
    *   **结构**:
        ```json
        {
            "action": "string (move, resize, close, minimize, maximize, restore)",
            "payload": {
                // "payload" 是可选的，具体取决于 action
                // "move" 示例:
                "x": "integer",
                "y": "integer",
                // "resize" 示例:
                "width": "integer",
                "height": "integer"
            }
        }
        ```
    *   **`move` 示例**:
        ```json
        {
            "action": "move",
            "payload": {
                "x": 200,
                "y": 250
            }
        }
        ```
    *   **`close` 示例**:
        ```json
        {
            "action": "close"
        }
        ```
*   **成功响应**:
    *   **代码**: `200 OK`
    *   **内容**: `application/json`
    ```json
    {
        "status": "success",
        "message": "Window moved successfully."
    }
    ```
*   **错误响应**:
    *   **代码**: `400 Bad Request` (例如，无效的 action 或 payload)
    *   **代码**: `404 Not Found`

### 5. 执行命令

*   **端点**: `POST /execute`
*   **描述**: 在服务器上执行新命令或启动新应用。
*   **请求体**: `application/json`
    *   **结构**:
        ```json
        {
            "command": "string (必需, 例如 'notepad.exe')",
            "args": ["string"], // 可选参数
            "async": "boolean (默认: true)" // 异步执行
        }
        ```
    *   **示例**:
        ```json
        {
            "command": "ping",
            "args": ["google.com", "-n", "4"]
        }
        ```
*   **成功响应**:
    *   **代码**: `202 Accepted`
    *   **内容**: `application/json`
    ```json
    {
        "status": "started",
        "pid": 54321,
        "command": "ping google.com -n 4"
    }
    ```
*   **错误响应**:
    *   **代码**: `400 Bad Request` (例如，未指定 command) 

---

### 6. 鼠标控制

*   **端点**: `POST /windows/{window_id}/mouse`
*   **描述**: 在指定窗口内模拟鼠标操作。坐标是相对于窗口左上角的。
*   **路径参数**:
    *   `window_id` (string, 必需): 窗口的唯一标识符。
*   **请求体**: `application/json`
    *   **结构**:
        ```json
        {
            "action": "string (move, click, double_click, right_click)",
            "x": "integer",
            "y": "integer"
        }
        ```
    *   **`click` 示例**:
        ```json
        {
            "action": "click",
            "x": 120,
            "y": 240
        }
        ```
*   **成功响应**:
    *   **代码**: `200 OK`
    *   **内容**: `application/json`
    ```json
    {
        "status": "success",
        "message": "Mouse action 'click' performed at (120, 240)."
    }
    ```
*   **错误响应**:
    *   **代码**: `400 Bad Request` (无效的 action 或缺少坐标)
    *   **代码**: `404 Not Found` (窗口未找到)

### 7. 键盘控制

*   **端点**: `POST /windows/{window_id}/keyboard`
*   **描述**: 向指定窗口发送键盘输入。
*   **路径参数**:
    *   `window_id` (string, 必需): 窗口的唯一标识符。
*   **请求体**: `application/json`
    *   **结构**:
        ```json
        {
            "action": "string (type_text, press_keys)",
            "payload": {
                // 用于 "type_text"
                "text": "string", 
                // 用于 "press_keys"
                "keys": ["string"] 
            }
        }
        ```
    *   **输入文本示例**:
        ```json
        {
            "action": "type_text",
            "payload": {
                "text": "hello world from api"
            }
        }
        ```
    *   **按快捷键示例 (Ctrl+S)**:
        ```json
        {
            "action": "press_keys",
            "payload": {
                "keys": ["ctrl", "s"]
            }
        }
        ```
    *   **按单个键示例 (回车)**:
        ```json
        {
            "action": "press_keys",
            "payload": {
                "keys": ["enter"]
            }
        }
        ```
*   **成功响应**:
    *   **代码**: `200 OK`
    *   **内容**: `application/json`
    ```json
    {
        "status": "success",
        "message": "Keyboard action performed."
    }
    ```
*   **错误响应**:
    *   **代码**: `400 Bad Request` (无效的 action 或 payload)
    *   **代码**: `404 Not Found` (窗口未找到) 